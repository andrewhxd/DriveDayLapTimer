#include <Arduino.h>
#include <RadioLib.h>
#include <SPI.h>
#include <esp_mac.h>
#include <TFMPlus.h>
#include "lora.h"

/*~~~~~Pin Mapping~~~~~*/

// TF Luna
#define TF_SDA_RX 6 // SDA/RXD
#define TF_SCL_TX 7 // SCL/TXD
// #define TF_CFG 3 <- Strapping Pin
// #define TF_MODE 2 <- GPIO Pin 2 Conflicts with GC1109 FEM

// Buttons (input)
#define RESET_BTN 42
#define COUNT_UP_BTN 0 // <-- use this for triggering laps for now PRG BUTTON

// Gym Timer Display outputs
#define RESET_SIG 4
#define COUNT_SIG 5

/*~~~~~TF Luna Setup~~~~~*/

TFMPlus tfmP;
HardwareSerial TFSerial(1);

#define LAP_TRIGGER_CM 300
#define LAP_REARM_MS 1000 // wait this long after a trigger before re-arming

static bool armed = true; // waiting for car, set to false while car is passing by
static unsigned long lastTriggerMs = 0;

int16_t dist = 0;
int16_t flux = 0;
int16_t temp = 0;

/*~~~~~Radio Configuration~~~~~*/

// Initialize SX1262 radio
// Make a custom SPI device because *of course* Heltec didn't use the default SPI pins
SPIClass spi(FSPI);
SPISettings spiSettings(2000000, MSBFIRST, SPI_MODE0); // Defaults, works fine
SX1262 radio = new Module(LORA_NSS_PIN, LORA_DIO1_PIN, LORA_RST_PIN, LORA_BUSY_PIN, spi, spiSettings);

/*~~~~~Global Variables~~~~~*/

uint32_t deviceId = 0;
volatile bool countFlag = false;
volatile bool receivedFlag = false;
volatile bool resetFlag = false;

// button debounce
unsigned long lastPressTime = 0;
const unsigned long debounceDelay = 300;

/*~~~~~Interrupts~~~~~*/

// This function should be called when button is pressed
//  It is placed in RAM to avoid Flash usage errors
void IRAM_ATTR countISR(void)
{
  countFlag = true;
}

void IRAM_ATTR receiveISR(void)
{
  // WARNING:  No Flash memory may be accessed from the IRQ handler: https://stackoverflow.com/a/58131720
  //  So don't call any functions or really do anything except change the flag
  receivedFlag = true;
}

void IRAM_ATTR resetISR(void)
{
  resetFlag = true;
}
/*~~~~~Helper Functions~~~~~*/

void setup()
{
  Serial.begin(115200);
  delay(2000);
  // button flag setup
  pinMode(COUNT_UP_BTN, INPUT_PULLUP); // wired to pullup for easier testing
  attachInterrupt(digitalPinToInterrupt(COUNT_UP_BTN), countISR, FALLING);

  pinMode(RESET_BTN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(RESET_BTN), resetISR, FALLING);

  pinMode(RESET_SIG, OUTPUT);
  pinMode(COUNT_SIG, OUTPUT);

  // TF Luna in UART mode, tx and rx are configured elsewhere
  // pinMode(TF_MODE, OUTPUT);
  // pinMode(TF_CFG, OUTPUT);

  // get device id from factory-burned eFuse base MAC (3 bytes)
  // IEEE 802 format is first 3 OUI, last 3 vendor unique
  uint8_t mac[6];
  esp_efuse_mac_get_default(mac);
  deviceId = ((uint32_t)mac[3] << 16) |
             ((uint32_t)mac[4] << 8) |
             mac[5];

  Serial.printf("Device ID: %06X\n", deviceId);

  /* Initialize TF Luna */
  // begin(baud, config, esp_rx, esp_tx): ESP RX = Luna TX, ESP TX = Luna RX
  TFSerial.begin(115200, SERIAL_8N1, TF_SCL_TX, TF_SDA_RX);
  delay(200);

  tfmP.begin(&TFSerial);
  Serial.println("TFMPlus initialized");

  /* Initialize Lora */
  lora_init(radio, spi);

  // set the function that will be called when a new packet is received
  radio.setDio1Action(receiveISR);

  // start continuous reception
  Serial.print("Beginning continuous reception...");
  int16_t state = radio.startReceive();
  if (state != RADIOLIB_ERR_NONE)
  {
    Serial.println("Starting reception failed");
  }
  Serial.println("Complete!");

  Serial.println("Ready. Press COUNT_UP_BTN to send packet.");
}

void loop()
{
  if (resetFlag)
  {
    resetFlag = false;
    digitalWrite(RESET_SIG, HIGH);
    delay(50);
    digitalWrite(RESET_SIG, LOW);
  }

  // if recieved this is basically only for the board with the timer plugged in.
  if (receivedFlag)
  {
    receivedFlag = false;

    uint32_t rxId = 0;
    bool gotPacket = lora_read_id(radio, rxId);

    if (gotPacket && rxId != deviceId) {
      digitalWrite(COUNT_SIG, HIGH);
      delay(50);
      digitalWrite(COUNT_SIG, LOW);
    }

    radio.startReceive();
  }

  // if you hit the button to trigger a lap
  if (countFlag)
  {
    countFlag = false;

    unsigned long now = millis();

    // check for debounce
    if (now - lastPressTime > debounceDelay)
    {
      lastPressTime = now;

      digitalWrite(COUNT_SIG, HIGH);
      delay(50);
      digitalWrite(COUNT_SIG, LOW);

      lora_send_id(radio, deviceId);
      radio.startReceive();
      receivedFlag = false; // discard TxDone IRQ that shares DIO1
    }
  }

  // now for laps triggered by tf-luna
  bool tfOk = tfmP.getData(dist, flux, temp);
  static unsigned long lastTfPrint = 0;
  if (millis() - lastTfPrint > 200)
  {
    lastTfPrint = millis();
    Serial.printf("ok=%d armed=%d dist=%d flux=%d\n", tfOk, armed, dist, flux);
  }
  // check if armed and dist > 0 as 0 is default when too far away
  if (armed && dist > 0 && dist <= LAP_TRIGGER_CM)
  {
    // car is passing by
    armed = false;
    lastTriggerMs = millis();
    Serial.printf("Lap! dist=%d\n", dist);

    // toggle display
    digitalWrite(COUNT_SIG, HIGH);
    delay(50);
    digitalWrite(COUNT_SIG, LOW);

    lora_send_id(radio, deviceId);
    radio.startReceive();
    receivedFlag = false; // discard TxDone IRQ that shares DIO1
  }
  else if (!armed && (millis() - lastTriggerMs) >= LAP_REARM_MS)
  {
    armed = true;
    Serial.println("Re-armed");
  }
}
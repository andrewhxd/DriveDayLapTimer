#include <Arduino.h>
#include <RadioLib.h>
#include <SPI.h>
#include <esp_mac.h>
#include <TFMPlus.h>

// function definitions:
void sendLapPacket();

/*~~~~~Pin Mapping~~~~~*/

// TF Luna
#define TF_SDA_RX 6 // SDA/RXD
#define TF_SCL_TX 7 // SCL/TXD
// #define TF_CFG 3
// #define TF_MODE 2 <- GPIO Pin 2 Conflicts with GC1109 FEM

// Buttons (input)
#define RESET_BTN 42
#define COUNT_UP_BTN 41 // <-- use this for triggering laps for now

// Gym Timer Display outputs
#define RESET_SIG 4
#define COUNT_SIG 5

/*~~~~~Hardware Definitions~~~~~*/

// These are hardware specific to the Heltec WiFi LoRa 32 V4
// Cite: https://resource.heltec.cn/download/WiFi_LoRa_32_V4/Schematic/HTIT-WB32LAF_V4.3.pdf
#define PRG_BUTTON 0
#define LORA_NSS_PIN 8
#define LORA_SCK_PIN 9
#define LORA_MOSI_PIN 10
#define LORA_MISO_PIN 11
#define LORA_RST_PIN 12
#define LORA_BUSY_PIN 13
#define LORA_DIO1_PIN 14

// GC1109 front-end enable (CSD)
#define FEM_EN 2

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

// make sure basestation is on the same channel
#define LORA_FREQ 915.0
#define LORA_BW 125.0
#define LORA_SF 9

/*~~~~~Global Variables~~~~~*/

uint32_t deviceId = 0;
volatile bool countFlag = false;

static uint32_t laps_sent = 0;

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

/*~~~~~Helper Functions~~~~~*/

void error_message(const char *message, int16_t state)
{
  Serial.printf("ERROR!!! %s with error code %d\n", message, state);
  while (true)
    ; // loop forever
}

void setup()
{
  Serial.begin(115200);
  delay(2000);
  // button flag setup
  pinMode(COUNT_UP_BTN, INPUT_PULLUP); // wired to pullup for easier testing
  attachInterrupt(digitalPinToInterrupt(COUNT_UP_BTN), countISR, FALLING);

  // Other pins (just initialize, don't use yet)
  pinMode(RESET_BTN, INPUT_PULLUP);

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
  TFSerial.begin(115200, SERIAL_8N1, TF_SDA_RX, TF_SCL_TX);
  delay(200);

  tfmP.begin(&TFSerial);
  Serial.println("TFMPlus initialized");

  /* Initialize Lora */
  // Set up SPI with our specific pins
  pinMode(FEM_EN, OUTPUT);
  digitalWrite(FEM_EN, HIGH); // GC1109 Permanently Enabled
  spi.begin(LORA_SCK_PIN, LORA_MISO_PIN, LORA_MOSI_PIN, LORA_NSS_PIN);

  Serial.print("Initializing radio...");
  int16_t state = radio.begin(LORA_FREQ, LORA_BW, LORA_SF, 5, 0x12, 22, 8);
  if (state != RADIOLIB_ERR_NONE)
  {
    error_message("Radio initialization failed", state);
  }

  state = radio.setCurrentLimit(140.0);
  if (state != RADIOLIB_ERR_NONE)
  {
    error_message("Current limit initialization failed", state);
  }

  state = radio.setDio2AsRfSwitch(true);
  if (state != RADIOLIB_ERR_NONE)
  {
    error_message("DIO2 RF switch initialization failed", state);
  }

  state = radio.explicitHeader();
  if (state != RADIOLIB_ERR_NONE)
  {
    error_message("Explicit header initialization failed", state);
  }

  state = radio.setCRC(2);
  if (state != RADIOLIB_ERR_NONE)
  {
    error_message("CRC initialization failed", state);
  }

  Serial.println("Complete!");
  Serial.println("Ready. Press COUNT_UP_BTN to send packet.");
}

void loop()
{
  if (countFlag)
  {
    countFlag = false;

    unsigned long now = millis();

    // check for debounce
    if (now - lastPressTime > debounceDelay)
    {
      lastPressTime = now;

      sendLapPacket();
    }
  }

  tfmP.getData(dist, flux, temp);
  // check if armed and dist > 0 as 0 is default when too far away
  if (armed && dist > 0 && dist <= LAP_TRIGGER_CM)
  {
    // car is passing by
    armed = false;
    lastTriggerMs = millis();
    Serial.printf("Lap! dist=%d\n", dist);
    sendLapPacket();
  }
  else if (!armed && (millis() - lastTriggerMs) >= LAP_REARM_MS)
  {
    armed = true;
    Serial.println("Re-armed");
  }
}

void sendLapPacket()
{
  laps_sent++;

  uint8_t data[3];

  data[0] = (deviceId >> 16) & 0xFF;
  data[1] = (deviceId >> 8) & 0xFF;
  data[2] = deviceId & 0xFF;

  int16_t state = radio.transmit(data, 3);

  if (state == RADIOLIB_ERR_NONE)
  {
    Serial.println("Packet sent!");
  }
  else
  {
    Serial.printf("Transmit failed, code %d\n", state);
  }
}
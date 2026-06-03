#include <Arduino.h>
#include <RadioLib.h>
#include <SPI.h>
#include <esp_mac.h>
#include <TFMPlus.h>
#include "lora.h"
#include "display.h"

/*~~~~~Pin Mapping~~~~~*/

// TF Luna
#define TF_SDA_RX 6 // SDA/RXD
#define TF_SCL_TX 7 // SCL/TXD

// Buttons (input)
#define RESET_BTN 42
#define COUNT_UP_BTN 0 // PRG button, manual lap trigger

// Gym Timer Display outputs
#define RESET_SIG 4
#define COUNT_SIG 5

/*~~~~~TF Luna Setup~~~~~*/

TFMPlus tfmP;
HardwareSerial TFSerial(1);

#define LAP_TRIGGER_CM 300
#define LAP_REARM_MS 1000  // shared debounce for both trigger paths
#define PULSE_MS 30        // gym timer pulse width
#define INTER_PULSE_MS 30  // gap between consecutive pulses so the timer registers both

static bool armed = true;
static unsigned long lastTriggerMs = 0;
static bool timer_running = false; // false until first lap passes the line

int16_t dist = 0;
int16_t flux = 0;
int16_t temp = 0;

/*~~~~~Radio Configuration~~~~~*/

SPIClass spi(FSPI);
SPISettings spiSettings(2000000, MSBFIRST, SPI_MODE0);
SX1262 radio = new Module(LORA_NSS_PIN, LORA_DIO1_PIN, LORA_RST_PIN, LORA_BUSY_PIN, spi, spiSettings);

// Initialize display
U8G2_SSD1306_128X64_NONAME_F_SW_I2C display(U8G2_R0, /* clock=*/OLED_SCL, /* data=*/OLED_SDA, /* reset=*/OLED_RESET); // All Boards without Reset of the Display

/*~~~~~Global Variables~~~~~*/

char* deviceType = "Remote-Sensor";
static uint32_t deviceId = 0;
volatile bool countFlag = false;
volatile bool resetFlag = false;
static uint32_t lap_count = 0;

/*~~~~~Interrupts~~~~~*/

void IRAM_ATTR countISR(void) { 
  countFlag = true; 
}

void IRAM_ATTR resetISR(void) { 
  resetFlag = true; 
}

/*~~~~~Helpers~~~~~*/

// Pulse a display signal pin.
static void pulse(uint8_t pin)
{
  digitalWrite(pin, HIGH);
  delay(PULSE_MS);
  digitalWrite(pin, LOW);
}

static void triggerLap(const char* source)
{
  armed = false;
  lastTriggerMs = millis();
  Serial.printf("Lap! (%s)\n", source);

  if (!timer_running)
  {
    // first time across the line: just start the timer
    pulse(COUNT_SIG);
    timer_running = true;
  }
  else
  {
    // stop -> reset -> restart, 
    pulse(COUNT_SIG);            // stop
    delay(INTER_PULSE_MS);
    pulse(RESET_SIG);            // reset
    delay(INTER_PULSE_MS);
    pulse(COUNT_SIG);            // start counting the next lap
  }

  lora_send_id(radio, deviceId);

  display_lap_count(display, lap_count);
  lap_count++; // increment after as first lap isn't a lap its just crossing gate
}

void setup()
{
  Serial.begin(115200);
  delay(500);

  // Attach GPIO Pins
  pinMode(COUNT_UP_BTN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(COUNT_UP_BTN), countISR, FALLING);

  pinMode(RESET_BTN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(RESET_BTN), resetISR, FALLING);

  pinMode(RESET_SIG, OUTPUT);
  pinMode(COUNT_SIG, OUTPUT);

  // Get Device ID 
  uint8_t mac[6];
  esp_efuse_mac_get_default(mac);
  deviceId = ((uint32_t)mac[3] << 16) |
             ((uint32_t)mac[4] << 8) |
             mac[5];
  Serial.printf("Device ID: %06X\n", deviceId);

  // Initialize TF-Luna
  TFSerial.begin(115200, SERIAL_8N1, TF_SCL_TX, TF_SDA_RX);
  delay(200);
  tfmP.begin(&TFSerial);
  tfmP.sendCommand(SET_FRAME_RATE, 250); // bump tf luna polling rate to 250
  Serial.println("TFMPlus initialized");

  // Setup Radio
  lora_init(radio, spi);

  // Start up Display
  display_init(display);
  display_logo(display, deviceType);
  delay(1000);
  display_waiting(display);

  Serial.println("Ready!");
}

void loop()
{
  if (resetFlag)
  {
    resetFlag = false;
    // full reset from the front-panel button: clear display and forget
    // that the timer was running so the next pass acts as "first lap"
    pulse(RESET_SIG);
    timer_running = false;
  }

  // re-arm (based on LAP_REARM_MS)
  if (!armed && (millis() - lastTriggerMs) >= LAP_REARM_MS)
  {
    armed = true;
    Serial.println("Re-armed");
  }

  // manual button trigger
  if (countFlag)
  {
    countFlag = false;
    if (armed) triggerLap("button");
  }

  // TF-Luna trigger (car passing)
  tfmP.getData(dist, flux, temp);
  if (armed && dist > 0 && dist <= LAP_TRIGGER_CM)
  {
    triggerLap("tf-luna");
  }
}

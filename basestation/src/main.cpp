#include <Arduino.h>
#include <RadioLib.h>
#include <SPI.h>
#include <esp_mac.h>
#include <map>
#include "lora.h"
#include "display.h"


// Initialize SX1262 radio
// Make a custom SPI device because *of course* Heltec didn't use the default SPI pins
SPIClass spi(FSPI);
SPISettings spiSettings(2000000, MSBFIRST, SPI_MODE0); // Defaults, works fine
SX1262 radio = new Module(LORA_NSS_PIN, LORA_DIO1_PIN, LORA_RST_PIN, LORA_BUSY_PIN, spi, spiSettings);

U8G2_SSD1306_128X64_NONAME_F_SW_I2C display(U8G2_R0, /* clock=*/OLED_SCL, /* data=*/OLED_SDA, /* reset=*/OLED_RESET); // All Boards without Reset of the Display

/*~~~~~Global Variables~~~~~*/
volatile bool receivedFlag = false;

/*~~~~~Timing State~~~~~*/
// The first ID we ever see becomes the lap marker (start/finish line).
// Subsequent sightings of that same ID close out a lap.
// Other IDs are treated as segments within the current lap.
bool lap_marker_set = false;
uint32_t lap_marker_id = 0;
uint32_t lap_count = 0;
uint32_t lap_start_ms = 0;
uint32_t last_lap_ms = 0; // duration of the most recently completed lap

// Per-lap segment timestamps: id -> millis() when seen this lap.
// Cleared on every lap rollover.
std::map<uint32_t, uint32_t> segment_times;

/*~~~~~Interrupts~~~~~*/
void IRAM_ATTR receiveISR(void)
{
  // WARNING:  No Flash memory may be accessed from the IRQ handler: https://stackoverflow.com/a/58131720
  //  So don't call any functions or really do anything except change the flag
  receivedFlag = true;
}

/*~~~~~Helper Functions~~~~~*/

void logo()
{
  display.clearBuffer();

  snprintf(display_str, sizeof(display_str), "Initializing");
  display.drawStr(20, 20, display_str);

  snprintf(display_str, sizeof(display_str), "Basestation");
  display.drawStr(20, 50, display_str);

  display.sendBuffer();
}

void setup()
{
  Serial.begin(115200);
  delay(2000);

  /* Initialize Lora */
  lora_init(radio, spi);

  // set the function that will be called when a new packet is received
  radio.setDio1Action(receiveISR);

  // start continuous reception
  Serial.print("Beginning continuous reception...");
  state = radio.startReceive();
  if (state != RADIOLIB_ERR_NONE)
  {
    error_message("Starting reception failed", state);
  }
  Serial.println("Complete!");

  /* Initialize Display */
  display_init(display)

  // draw startup logo
  logo();
  delay(3000);
  display.clear();
}

void loop()
{
  if (recievedFlag) {
    recievedFlag = false;
    uint32_t id;
    
    // read radio data 
    if (lora_read_id(radio,id)) {
      // get current time to compare
      uint32_t now = millis(); 

      // if this is first id/gate we see
      if (!lap_marker_set) {
        lap_marker_set = true;
        lap_marker_id = id;
        lap_start_ms = now;

        Serial.printf("\tLap marker registered: %06X\n", id);
      }
      else if (id == lap_marker_id) { // if we find the first gate again, end lap
        uint32_t elapsed = now - lap_start_ms
        last_lap_ms = elapsed;

        lap_count++;

        Serial.printf("LAP %lu: %lu ms\n", lap_count, elapsed);

        // dump segment times
        for (uint32_t& segment : segment_times) {
          uint32_t split = segment.second - lap_start_ms;
          Serial.printf("\t  segment %06X: %lu ms\n", segment.first, split);
        }

        segment_times.clear();

        lap_start_ms = now;
      }
      else {
        uint32_t split = now - lap_start_ms;
        segment_times[id] = now;

        Serial.printf("Segment %06X @ %lu ms\n", id, split);
      }
    }
  }
}

#include <Arduino.h>
#include <RadioLib.h>
#include <SPI.h>
#include <esp_mac.h>
#include <map>
#include "lora.h"
#include "display.h"
#include "gymtimer.h"

// Gym Timer Display outputs
#define RESET_SIG 4
#define COUNT_SIG 5

static bool timer_running = false;
static bool first_segment = true;

// Initialize SX1262 radio
// Make a custom SPI device because *of course* Heltec didn't use the default SPI pins
SPIClass spi(FSPI);
SPISettings spiSettings(2000000, MSBFIRST, SPI_MODE0); // Defaults, works fine
SX1262 radio = new Module(LORA_NSS_PIN, LORA_DIO1_PIN, LORA_RST_PIN, LORA_BUSY_PIN, spi, spiSettings);

// Initialize display
U8G2_SSD1306_128X64_NONAME_F_SW_I2C display(U8G2_R0, /* clock=*/OLED_SCL, /* data=*/OLED_SDA, /* reset=*/OLED_RESET); // All Boards without Reset of the Display

/*~~~~~Global Variables~~~~~*/
volatile bool receivedFlag = false;
char* deviceType = "Base Station";

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
static void triggerLap()
{
  if (!timer_running)
  {
    // first time across the line: just start the timer
    gymtimer_pulse(COUNT_SIG);
    timer_running = true;
  }
  else
  {
    // Only reset -> restart, the first segment will stop
    uint8_t seq[] = {RESET_SIG, COUNT_SIG};
    gymtimer_sequence(seq, 2);
  }
}

void setup()
{
  Serial.begin(115200);
  delay(2000);

  gymtimer_init(COUNT_SIG, RESET_SIG);

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

  // start up display
  display_init(display);
  display_logo(display, deviceType);
  delay(1000);
  display_waiting(display);

  Serial.println("Ready!");
}

void loop()
{
  gymtimer_update(); // advance current display pulse state 

  if (receivedFlag) {
    receivedFlag = false;
    uint32_t id;

    // read radio data
    if (lora_read_id(radio,id)) {
      // get current time to compare
      uint32_t now = millis();

      Serial.printf("RX %06X  RSSI=%.1f dBm  SNR=%.1f dB\n",
                    id, radio.getRSSI(), radio.getSNR());

      // if this is first id/gate we see
      if (!lap_marker_set) {
        lap_marker_set = true;
        lap_marker_id = id;
        lap_start_ms = now;

        Serial.printf("\tLap marker registered: %06X\n", id);

        // toggle the display (only plugged in for acceleration test)
        triggerLap();

        // temp display after waiting to show lap marker registered
        display_info(display, 0, 0);
      }
      else if (id == lap_marker_id) { // if we find the first gate again, end lap
        uint32_t elapsed = now - lap_start_ms;
        last_lap_ms = elapsed;

        lap_count++;

        Serial.printf("LAP %lu: %.3f s\n", lap_count, elapsed / 1000.0);
        display_info(display, last_lap_ms, lap_count);

        // dump segment times
        for (auto& segment : segment_times) {
          uint32_t split = segment.second - lap_start_ms;
          Serial.printf("\t  segment %06X: %.3f s\n", segment.first, split / 1000.0);
        }
        segment_times.clear();
        lap_start_ms = now;

        first_segment = true; // rearm first segment
        triggerLap();         // pulse display again for accel
      }
      else {
        // add segment
        uint32_t split = now - lap_start_ms;
        segment_times[id] = now;

        Serial.printf("Segment %06X @ %.3f s\n", id, split / 1000.0);
        
        // first segment
        if (first_segment) {
          first_segment = false;
          gymtimer_pulse(COUNT_SIG);
        }
      }
    }
  }
}

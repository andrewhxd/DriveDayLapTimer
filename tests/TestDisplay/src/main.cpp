#include <Arduino.h>
#include <U8g2lib.h>

#define VEXT_CTRL 36
#define OLED_RESET 21
#define OLED_SDA 17
#define OLED_SCL 18
U8G2_SSD1306_128X64_NONAME_F_SW_I2C display(U8G2_R0, /* clock=*/OLED_SCL, /* data=*/OLED_SDA, /* reset=*/OLED_RESET); // All Boards without Reset of the Display

// Screen drawing locations
#define X_MAX 128
#define Y_MAX 64
static uint8_t iteration_count = 0;
static uint32_t x_coor = 0;
static uint32_t y_coor = 10;
static int8_t x_rate = 4;
static int8_t y_rate = 4;

// String to draw on screen
static char display_str[80] = {0};

void setup() {
  Serial.begin(115200);
  delay(2000);

  // Initialize the display 
  pinMode(VEXT_CTRL, OUTPUT);
  digitalWrite(VEXT_CTRL, LOW);

  display.begin();
  display.setContrast(200);
  display.setFont(u8g2_font_ncenB10_tr);
}

void loop() {
  iteration_count++;

  // Draw text on screen
  snprintf(display_str, 80, "Itr: %03d", iteration_count);
  display.clearBuffer();
  display.drawStr(x_coor, y_coor, display_str);
  display.sendBuffer();

  // Update text location
  x_coor += x_rate;
  if (x_coor + x_rate > X_MAX || x_coor + x_rate < 0)
  {
    x_rate *= -1;
  }
  y_coor += y_rate;
  if (y_coor + y_rate > Y_MAX || y_coor + y_rate < 0)
  {
    y_rate *= -1;
  }

  // Sleep
  delay(100);
}

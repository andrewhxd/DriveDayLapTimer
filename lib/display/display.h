#pragma once
#include <U8g2lib.h>

// Heltec v4 OLED Pins
#define VEXT_CTRL 36
#define OLED_RESET 21
#define OLED_SDA 17
#define OLED_SCL 18

void display_init(U8G2& display);

// print two lines of text for start up
void display_logo(U8G2& display, char* deviceType);

// print two lines with info value
void display_info(U8G2& display, uint32_t last_lap_ms, uint32_t lap_count);

// print just lap count
void display_lap_count(U8G2& display, uint32_t lap_count);

// waiting for lap
void display_waiting(U8G2& display);
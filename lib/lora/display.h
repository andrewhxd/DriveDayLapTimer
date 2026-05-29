#pragma once
#include <U8g2lib.h>

// Heltec v4 OLED Pins
#define VEXT_CTRL 36
#define OLED_RESET 21
#define OLED_SDA 17
#define OLED_SCL 18

void display_init(U8G2& display);

// print two lines of text for start up
void display_logo(U8G2& display, char* str1, char* str2);

// print two lines with info value
void display_info(U8G2& display, char* str1, char* val1, char* str2, char* val2);
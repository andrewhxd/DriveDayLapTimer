#include <Arduino.h>
#include "display.h"

static char display_str[80] = {0};

void display_init(U8G2& display) {
    // Initialize the display
    pinMode(VEXT_CTRL, OUTPUT);
    digitalWrite(VEXT_CTRL, LOW);

    display.begin();
    display.setContrast(200);
    display.setFont(u8g2_font_ncenB10_tr);
}

void display_logo(U8G2& display, char* deviceType) {
    display.clearBuffer();

    snprintf(display_str, sizeof(display_str), "Initializing");
    display.drawStr(20, 20, display_str);

    snprintf(display_str, sizeof(display_str), deviceType);
    display.drawStr(20, 50, display_str);

    display.sendBuffer();
}

void display_info(U8G2& display, uint32_t last_lap_ms, uint32_t lap_count) {
    display.clearBuffer();

    snprintf(display_str, sizeof(display_str), "Last Lap: %.3f s", last_lap_ms / 1000.0f);
    display.drawStr(10, 20, display_str);

    snprintf(display_str, sizeof(display_str), "Lap Count: %lu", lap_count);
    display.drawStr(10, 50, display_str);

    display.sendBuffer();
}

void display_lap_count(U8G2 &display, uint32_t lap_count) {
    display.clearBuffer();

    snprintf(display_str, sizeof(display_str), "Lap Count: %lu", lap_count);
    display.drawStr(10, 30, display_str);

    display.sendBuffer();
}  

void display_waiting(U8G2 &display)
{
    display.clearBuffer();

    snprintf(display_str, sizeof(display_str), "Waiting...");
    display.drawStr(20, 30, display_str);

    display.sendBuffer();
}
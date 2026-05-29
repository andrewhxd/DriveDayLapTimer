#include <Arduino.h>
#include "display.h"

void display_init(U8G2& display) {
    // Initialize the display
    pinMode(VEXT_CTRL, OUTPUT);
    digitalWrite(VEXT_CTRL, LOW);

    display.begin();
    display.setContrast(200);
    display.setFont(u8g2_font_ncenB10_tr);
}

void display_logo
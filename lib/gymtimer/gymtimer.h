#pragma once
#include <Arduino.h>

// Instead of delay(), you start off a pulse 
// and then call gymtimer_update() every loop() to let it play out.

// Set both signal pins as OUTPUT.
void gymtimer_init(uint8_t countPin, uint8_t resetPin);

// Pulse one pin once (HIGH then LOW).
void gymtimer_pulse(uint8_t pin);

// Pulse several pins in order, e.g. {COUNT, RESET, COUNT}.
void gymtimer_sequence(const uint8_t* pins, uint8_t count);

// Call this every loop(). It advances the current pulse using millis().
void gymtimer_update();

// True while a pulse / sequence is still playing.
bool gymtimer_busy();

#include "gymtimer.h"

#define PULSE_MS 30 // pulse width AND the gap between pulses (both 30ms)

static uint8_t pins[4];           // the pins to pulse, in order
static uint8_t pin_count = 0;     // how many pins in the sequence
static uint8_t current = 0;       // which pin we're on
static bool pin_high = false;     // is the current pin HIGH right now
static unsigned long last_ms = 0; // when we last changed a pin

void gymtimer_init(uint8_t countPin, uint8_t resetPin)
{
  pinMode(countPin, OUTPUT);
  pinMode(resetPin, OUTPUT);
}

void gymtimer_sequence(const uint8_t* seq, uint8_t count)
{
  // if a pulse was still running, end it so no pin gets stuck HIGH
  if (pin_high)
  {
    digitalWrite(pins[current], LOW);
  }

  for (uint8_t i = 0; i < count && i < 4; i++)
  {
    pins[i] = seq[i];
  }
  pin_count = count;
  current = 0;

  // start the first pulse now
  digitalWrite(pins[0], HIGH);
  pin_high = true;
  last_ms = millis();
}

void gymtimer_pulse(uint8_t pin)
{
  gymtimer_sequence(&pin, 1);
}

void gymtimer_update()
{
  if (current >= pin_count)
  {
    return; // nothing playing
  }

  if (millis() - last_ms < PULSE_MS)
  {
    return; // not time to change yet
  }

  last_ms = millis();

  if (pin_high)
  {
    // pin is on -> turn it off and move to the next pin
    digitalWrite(pins[current], LOW);
    pin_high = false;
    current++;
  }
  else
  {
    // gap is over -> turn the next pin on
    digitalWrite(pins[current], HIGH);
    pin_high = true;
  }
}

bool gymtimer_busy()
{
  return current < pin_count;
}

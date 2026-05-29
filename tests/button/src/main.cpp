#include <Arduino.h>

#define COUNT_BTN 41
#define RESET_BTN 42

void setup() {
  Serial.begin(115200);
  delay(2000);

  pinMode(COUNT_BTN, INPUT_PULLUP);
  pinMode(RESET_BTN, INPUT_PULLUP);
}

void loop() {
  // could attach button to interrupts
  if (digitalRead(COUNT_BTN) == LOW) {
    Serial.println("COUNT pressed");
  }
  if (digitalRead(RESET_BTN) == LOW) {
    Serial.println("RESET pressed");
  }

  delay(200); 
}

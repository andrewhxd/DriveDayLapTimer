#include <Arduino.h>

#define COUNT_UP 5
#define RESET 4

void setup() {
  Serial.begin(115200);
  delay(2000);

  pinMode(COUNT_UP, OUTPUT);
  pinMode(RESET, OUTPUT);
}

void loop() {
  // pin has to send a short signal 
  // and go back to low, delay is currently 100ms
  // toggle on the timer
  digitalWrite(COUNT_UP, HIGH);
  delay(100);
  digitalWrite(COUNT_UP, LOW);
  Serial.println("Starting");

  delay(2000);

  // stop timer after delay
  digitalWrite(COUNT_UP, HIGH);
  delay(100);
  digitalWrite(COUNT_UP, LOW);
  Serial.println("Stopped");

  delay(2000);

  // reset timer
  digitalWrite(RESET, HIGH);
  delay(100);
  digitalWrite(RESET, LOW);
  Serial.println("Reset");
  
  delay(2000);
}

#include <Arduino.h>

#define VBAT_PIN 1
#define ADC_CTRL 37

void setup()
{
  Serial.begin(115200);
  delay(1000);

  pinMode(ADC_CTRL, OUTPUT);
  digitalWrite(ADC_CTRL, HIGH); // enable battery voltage divider

  analogReadResolution(12);
  analogSetPinAttenuation(VBAT_PIN, ADC_11db);

  Serial.println("Heltec V4 Battery Test");
}

void loop()
{
  digitalWrite(ADC_CTRL, HIGH);
  delay(5);

  int raw = analogRead(VBAT_PIN);

  // Try 4.01 first; adjust later using a multimeter
  float voltage = (raw / 4095.0) * 3.3 * 4.01;

  Serial.print("Raw: ");
  Serial.print(raw);
  Serial.print("  Voltage: ");
  Serial.print(voltage, 2);
  Serial.println(" V");

  delay(1000);
}
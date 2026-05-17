#include <Arduino.h>
#include <TFMPlus.h>

#define TF_RX 7 // ESP RX  <- Luna TXD (SCL/TXD)
#define TF_TX 6 // ESP TX  -> Luna RXD (SDA/RXD)

TFMPlus tfmP;
HardwareSerial TFSerial(1);

int16_t dist, flux, temp;

void setup()
{
  Serial.begin(115200);
  TFSerial.begin(115200, SERIAL_8N1, TF_RX, TF_TX);
  tfmP.begin(&TFSerial);
}

void loop()
{
  if (tfmP.getData(dist, flux, temp))
  {
    Serial.printf("Dist: %d cm | Flux: %d | Temp: %d\n", dist, flux, temp);
  }
  delay(100);
}

#pragma once
#include <RadioLib.h>
#include <SPI.h>

// Heltec WiFi LoRa 32 V4 pins
#define LORA_NSS_PIN 8
#define LORA_SCK_PIN 9
#define LORA_MOSI_PIN 10
#define LORA_MISO_PIN 11
#define LORA_RST_PIN 12
#define LORA_BUSY_PIN 13
#define LORA_DIO1_PIN 14

// GC1109 front-end enable for heltec v4
#define FEM_EN 2

// lora radio settings
#define LORA_FREQ 915.0
#define LORA_BW 125.0
#define LORA_SF 7

// main program initializes the radio still
// ADD lora radio settings to init
void lora_init(SX1262& radio, SPIClass& spi); 

// sends last 3 bytes id
void lora_send_id(SX1262& radio, uint32_t id);

// modify id to hold data from packet if function returns true
bool lora_read_id(SX1262& radio, uint32_t& id);
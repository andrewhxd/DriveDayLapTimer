#include "lora.h"
#include <Arduino.h>

static void error_message(const char *message, int16_t state)
{
    Serial.printf("ERROR!!! %s with error code %d\n", message, state);
    while (true)
        ; // loop forever
}

void lora_init(SX1262& radio, SPIClass& spi) {
    // turn on v4 power amp
    pinMode(FEM_EN, OUTPUT);
    digitalWrite(FEM_EN, HIGH);

    // Set up SPI with our specific pins
    spi.begin(LORA_SCK_PIN, LORA_MISO_PIN, LORA_MOSI_PIN, LORA_NSS_PIN);

    Serial.print("Initializing radio...");
    int16_t state = radio.begin(LORA_FREQ, LORA_BW, LORA_SF, 5, 0x12, 22, 8);
    if (state != RADIOLIB_ERR_NONE)
    {
        error_message("Radio initialization failed", state);
    }

    state = radio.setCurrentLimit(140.0);
    if (state != RADIOLIB_ERR_NONE)
    {
        error_message("Current limit initialization failed", state);
    }

    state = radio.setDio2AsRfSwitch(true);
    if (state != RADIOLIB_ERR_NONE)
    {
        error_message("DIO2 RF switch initialization failed", state);
    }

    state = radio.explicitHeader();
    if (state != RADIOLIB_ERR_NONE)
    {
        error_message("Explicit header initialization failed", state);
    }

    state = radio.setCRC(2);
    if (state != RADIOLIB_ERR_NONE)
    {
        error_message("CRC initialization failed", state);
    }

    Serial.println("Radio Initalized");
}

void lora_send_id(SX1262& radio, uint32_t id) {
    uint8_t data[3];

    data[0] = (id >> 16) & 0xFF;
    data[1] = (id >> 8) & 0xFF;
    data[2] = id & 0xFF;

    int16_t state = radio.transmit(data, 3);

    if (state == RADIOLIB_ERR_NONE) {
        Serial.println("Packet sent!");
    }
    else {
        Serial.printf("Transmit failed, code %d\n", state);
    }
}

bool lora_read_id(SX1262& radio, uint32_t& id) {
    // data buffer
    uint8_t data[3];

    int16_t state = radio.readData(data, sizeof(data));

    if (state == RADIOLIB_ERR_NONE) {
        Serial.println("Packet Recieved!");
        id = (data[0] << 16) |
             (data[1] << 8) |
             data[2];
        
        return true;
    }
    else if (state == RADIOLIB_ERR_RX_TIMEOUT)
    {
        // timeout occurred while waiting for a packet
        Serial.println("timeout!");
    }
    else if (state == RADIOLIB_ERR_CRC_MISMATCH)
    {
        // packet was received, but is malformed
        Serial.println("CRC error!");
    }
    else
    {
        // some other error occurred
        Serial.print("failed, code ");
        Serial.println(state);
    }
}
#include "lora.h"
#include <Arduino.h>

static void error_message(const char *message, int16_t state)
{
    Serial.printf("ERROR!!! %s with error code %d\n", message, state);
    while (true)
        ; // loop forever
}

// non-blocking transmit state
static volatile bool tx_done = false; // set from DIO1 ISR when packet is out
static bool tx_active = false;        // a startTransmit() is in flight

static void IRAM_ATTR lora_tx_isr(void)
{
    tx_done = true;
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

    // fire lora_tx_isr() on DIO1 when a transmission completes
    radio.setPacketSentAction(lora_tx_isr);

    Serial.println("Radio Initalized");
}

void lora_send_id(SX1262& radio, uint32_t id) {
    uint8_t data[3];

    data[0] = (id >> 16) & 0xFF;
    data[1] = (id >> 8) & 0xFF;
    data[2] = id & 0xFF;

    // if a previous send never got serviced, close it out first so the
    // radio isn't left mid-transmit
    if (tx_active) {
        radio.finishTransmit();
        tx_active = false;
    }

    tx_done = false;
    int16_t state = radio.startTransmit(data, 3);

    if (state == RADIOLIB_ERR_NONE) {
        tx_active = true; // completion handled in lora_service()
    }
    else {
        Serial.printf("Transmit start failed, code %d\n", state);
    }
}

void lora_service(SX1262& radio) {
    if (tx_active && tx_done) {
        tx_active = false;
        radio.finishTransmit(); // clears IRQ flags, returns radio to standby
        Serial.println("Packet sent!");
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
    return false;
}
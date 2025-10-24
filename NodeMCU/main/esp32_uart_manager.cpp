/*
 * Author: IlidioMSilva
 * Project: Sensor integration with Zephyr
 * Note: Educational/portfolio example
 */

/**
 * @brief Initialize the UART manager thread.
 *
 * Sets up UART on pins defined by TX_PIN/RX_PIN and prepares the
 * NodeMCU to send snapshot commands periodically.
 *
 * Command handled:
 *  - 0x01: Snapshot request
 *
 * Logs only go to Serial for debug purposes.
 */

#include "esp32_uart_manager.h"

#define NRF_BAUDRATE 115200
#define TX_PIN 17  // ESP32 TX → nRF52 RX
#define RX_PIN 16  // ESP32 RX ← nRF52 TX

HardwareSerial nrfSerial(1);  // UART1 for nRF52

#define CMD_SNAPSHOT 0x01
#define SEND_INTERVAL_MS 30000  // 30 seconds

static unsigned long last_send = 0;

//print received bytes in HEX
static void printHex(uint8_t *buf, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (buf[i] < 16) Serial.print('0');
        Serial.print(buf[i], HEX);
        Serial.print(' ');
    }
    Serial.println();
}

//convert two bytes to int16_t (big-endian)
static int16_t bytes_to_int16(uint8_t hi, uint8_t lo) {
    return (int16_t)((hi << 8) | lo);
}

/**
 * @brief Initialize the UART manager.
 *
 * Sets up UART1 for communication with the nRF52.
 */
void uart_manager_init(void) {
    nrfSerial.begin(NRF_BAUDRATE, SERIAL_8N1, RX_PIN, TX_PIN);
    Serial.printf("[ESP32] UART1 started on RX=%d TX=%d @%d baud\n", RX_PIN, TX_PIN, NRF_BAUDRATE);
}

/**
 * @brief Update function for the UART manager.
 *
 * Sends snapshot command every SEND_INTERVAL_MS and reads any
 * available response from nRF52.
 */
void uart_manager_update(void) {
    unsigned long now = millis();

    //Send snapshot comand periodically
    if (now - last_send >= SEND_INTERVAL_MS) {
        last_send = now;

        uint8_t cmd[2] = {CMD_SNAPSHOT, '\n'};
        nrfSerial.write(cmd, 2);
        Serial.println("[ESP32] Sent 0x01 snapshot command");
    }

    //check for incoming response
    static uint8_t rx_buf[33]; // 32 bytes + terminator
    size_t idx = 0;

    while (nrfSerial.available() && idx < sizeof(rx_buf)) {
        rx_buf[idx++] = nrfSerial.read();
    }

    if (idx > 0) {
        Serial.print("[ESP32] Received (HEX): ");
        printHex(rx_buf, idx);

        // parse snapshot fields locally
        if (rx_buf[0] == CMD_SNAPSHOT && idx >= 19) { // ensure enough bytes
            int16_t dht_temp = bytes_to_int16(rx_buf[1], rx_buf[2]);
            int16_t dht_hum  = bytes_to_int16(rx_buf[3], rx_buf[4]);
            int16_t bme_temp = bytes_to_int16(rx_buf[5], rx_buf[6]);
            int16_t bme_hum  = bytes_to_int16(rx_buf[7], rx_buf[8]);
            int32_t bme_pres = ((int32_t)rx_buf[9] << 24) | ((int32_t)rx_buf[10] << 16) |
                               ((int32_t)rx_buf[11] << 8) | rx_buf[12];

            int16_t t_min = bytes_to_int16(rx_buf[11], rx_buf[12]);
            int16_t t_max = bytes_to_int16(rx_buf[13], rx_buf[14]);
            int16_t h_min = bytes_to_int16(rx_buf[15], rx_buf[16]);
            int16_t h_max = bytes_to_int16(rx_buf[17], rx_buf[18]);

            //to remove later
            Serial.printf("[DEBUG] DHT11: Temp=%d C, Hum=%d %%\n", dht_temp, dht_hum);
            Serial.printf("[DEBUG] BME280: Temp=%.2f C, Hum=%.2f %%, Pres=%.2f hPa\n",
                          bme_temp / 100.0, bme_hum / 100.0, bme_pres / 100.0);
            Serial.printf("[DEBUG] Thresholds: T_min=%d T_max=%d, H_min=%d H_max=%d\n",
                          t_min, t_max, h_min, h_max);
        }
    }
}

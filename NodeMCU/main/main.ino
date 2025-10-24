
/*
 * Author: IlidioMSilva
 * Project: Sensor integration with Zephyr
 * Note: Educational/portfolio example
 */
 
/**
 * @brief Main sketch for NodeMCU ESP32 UART snapshot client.
 *
 * Initializes the UART manager and periodically sends the snapshot
 * command (0x01) to the nRF52. Received responses are printed in HEX
 * for debug purposes.
 */

#include <Arduino.h>
#include "esp32_uart_manager.h"

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n[ESP32] UART Snapshot Client Started");

    uart_manager_init();
}

void loop() {
    uart_manager_update();
}

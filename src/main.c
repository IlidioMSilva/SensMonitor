/*
 * Author: IlidioMSilva
 * Project: Sensor integration with Zephyr
 * Note: Educational/portfolio example
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "sensor_data.h"
#include "ble_manager.h"
#include "uart_manager.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

int main(void)
{
     LOG_INF("System started");

    if (sensors_init() != 0) {
        LOG_ERR("Error initializaing sensors!");
    } else {
        LOG_INF("Sensors initialized!");
    }

    // Initialize UART manager for communication with ESP32/NodeMCU
    uart_manager_init();

    // Initialize Bluetooth and BLE services
    ble_manager_init();
   
    // Main loop: sleep forever, all work handled by BLE and timer
    while (1) {
        k_sleep(K_FOREVER);
    }

    return 0;
}
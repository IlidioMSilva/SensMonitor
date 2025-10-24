/*
 * Author: IlidioMSilva
 * Project: Sensor integration with Zephyr
 * Note: Educational/portfolio example
 */

#ifndef BME280_SERVICE_H
#define BME280_SERVICE_H

#include <stdint.h>
#include <zephyr/bluetooth/conn.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize BME280 BLE GATT service and register characteristics.
 */
void bme280_service_init(void);

/**
 * @brief Update the temperature characteristic (hundredths of °C).
 *
 * @param new_temperature Temperature value (e.g., 2350 = 23.50 °C)
 */
void bme280_update_temperature(int16_t new_temperature);

/**
 * @brief Update the humidity characteristic (hundredths of %RH).
 *
 * @param new_humidity Humidity value (e.g., 4850 = 48.50 %)
 */
void bme280_update_humidity(int16_t new_humidity);

/**
 * @brief Manually update the temperature/humidity thresholds.
 *
 * @param new_thresholds Array of 4 thresholds: [temp_min, temp_max, hum_min, hum_max]
 */
void bme280_update_thresholds(int16_t new_thresholds[4]);

/**
 * @brief Set current BLE connection pointer (for notifications).
 *
 * @param conn Pointer to current bt_conn.
 */
void bme280_service_set_conn(struct bt_conn *conn);

#ifdef __cplusplus
}
#endif

#endif // BME280_SERVICE_H

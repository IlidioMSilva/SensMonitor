/*
 * Author: IlidioMSilva
 * Project: Sensor integration with Zephyr
 * Note: Educational/portfolio example
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/drivers/gpio.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "ble_manager.h"
#include "bme280_service.h"
#include "sensor_data.h"

LOG_MODULE_REGISTER(ble_manager, LOG_LEVEL_INF);

#define UPDATE_INTERVAL_MS 5000  // Sensor update interval in milliseconds

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);

static struct bt_conn *default_conn;
static struct bt_conn *current_conn = NULL;  // Current BLE connection reference

static struct k_timer update_timer;

/**
 * @brief Periodic function to read sensor values and update BLE characteristics.
 * 
 * @param timer Pointer to the timer structure.
 */
static void update_sensor_values(struct k_timer *timer)
{
    sensor_snapshot_t snapshot;
    sensor_data_get_snapshot(&snapshot);

    // Update temperature and humidity (scaled by 100 for BLE)
    bme280_update_temperature((int32_t)(snapshot.bme_data.temperature * 100));
    bme280_update_humidity((int32_t)(snapshot.bme_data.humidity * 100));

    // Log values in human-readable format
    int temp_raw = (int)(snapshot.bme_data.temperature * 100);
    int temp_int = temp_raw / 100;
    int temp_frac = abs(temp_raw % 100);
    int hum_int = (int)snapshot.bme_data.humidity;
    int hum_frac = (int)((snapshot.bme_data.humidity - hum_int) * 100);

    LOG_INF("BLE Update: Temp=%d.%02d C, Hum=%d.%02d%%", temp_int, temp_frac, hum_int, hum_frac);
}

/**
 * @brief Store BLE connection reference for use elsewhere.
 * 
 * @param conn Pointer to the BLE connection structure.
 */
void ble_manager_set_connection(struct bt_conn *conn)
{
    if (current_conn) {
        bt_conn_unref(current_conn);
        current_conn = NULL;
    }

    if (conn) {
        current_conn = bt_conn_ref(conn);
    }

    bme280_service_set_conn(conn);
}

/* ---------- Bluetooth connection callbacks ---------- */

/*
* @brief Callback invoked when a new connection is established.
*
* @param conn Pointer to the connection object.
* @param err HCI error code. Zero indicates success.
*/
static void connected(struct bt_conn *conn, uint8_t err)
{
    if (err) {
        LOG_ERR("Connection failed (err %u)", err);
        return;
    }

    LOG_INF("Connected");
    gpio_pin_set(led.port, led.pin, 1);

    default_conn = bt_conn_ref(conn);
    ble_manager_set_connection(default_conn);
}

/*
* @brief Callback invoked when a connection is disconnected.
*
* @param conn Pointer to the connection object.
* @param reason Disconnection reason.
*/
static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    LOG_INF("Disconnected (reason %u)", reason);
    gpio_pin_set(led.port, led.pin, 0);

    if (default_conn) {
        bt_conn_unref(default_conn);
        default_conn = NULL;
    }
    ble_manager_set_connection(NULL);
}

static struct bt_conn_cb conn_callbacks = {
    .connected = connected,
    .disconnected = disconnected,
};

/* ---------- Initialization ---------- */

/**
 * @brief Initialize the BLE manager: Bluetooth stack, advertising, services, etc.
 *
 * @return 0 on success, negative error code on failure.
 */
int ble_manager_init(void)
{
    int err;

    // Configure LED (optional visual indicator)
    if (!device_is_ready(led.port)) {
        LOG_ERR("LED GPIO device not ready");
        return -ENODEV;
    }

    err = gpio_pin_configure(led.port, led.pin, GPIO_OUTPUT_INACTIVE);
    if (err) {
        LOG_ERR("Failed to configure LED pin (err %d)", err);
        return err;
    }
    gpio_pin_set(led.port, led.pin, 0);  // LED off initially

    // Enable Bluetooth stack
    err = bt_enable(NULL);
    if (err) {
        LOG_ERR("Bluetooth init failed (err %d)", err);
        return err;
    }
    LOG_INF("Bluetooth initialized");

    // Register connection callbacks
    bt_conn_cb_register(&conn_callbacks);

    // Initialize BME280 BLE service
    bme280_service_init();

    // Generate unique device name based on BLE address
    size_t count = 1;
    bt_addr_le_t addr;
    bt_id_get(&addr, &count);
    if (count == 0) {
        LOG_ERR("No BLE addresses found");
        return -ENOENT;
    }

    char dev_name[32];
    snprintf(dev_name, sizeof(dev_name), "nRF52 Sensor-%02X%02X",
             addr.a.val[4], addr.a.val[5]);

    err = bt_set_name(dev_name);
    if (err) {
        LOG_ERR("Failed to set device name (err %d)", err);
        return err;
    }
    LOG_INF("Device name set: %s", dev_name);

    // Define advertising data
    const struct bt_data ad[] = {
        BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
        BT_DATA(BT_DATA_NAME_COMPLETE, dev_name, strlen(dev_name)),
    };

    // Start advertising (connectable)
    err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad), NULL, 0);
    if (err) {
        LOG_ERR("Advertising start failed (err %d)", err);
        return err;
    }
    LOG_INF("Advertising started!");

    // Start periodic sensor update timer
    k_timer_init(&update_timer, update_sensor_values, NULL);
    k_timer_start(&update_timer, K_MSEC(UPDATE_INTERVAL_MS), K_MSEC(UPDATE_INTERVAL_MS));

    LOG_INF("BLE Manager ready!");

    return 0;
}

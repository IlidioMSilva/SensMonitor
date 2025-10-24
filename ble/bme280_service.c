/*
 * Author: IlidioMSilva
 * Project: Sensor integration with Zephyr
 * Note: Educational/portfolio example
 */

#include <zephyr/logging/log.h>
#include <zephyr/types.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/gatt.h>
#include <string.h>

#include "bme280_service.h"

LOG_MODULE_REGISTER(bme280_service, LOG_LEVEL_INF);

// UUID definitions (128-bit UUIDs)
#define BT_UUID_BME280_SERVICE_VAL \
    BT_UUID_128_ENCODE(0x9f06a2b3, 0x4f2d, 0x4b87, 0x89d4, 0x16eaedbe0000ULL)
#define BT_UUID_BME280_TEMP_VAL \
    BT_UUID_128_ENCODE(0x9f06a2b3, 0x4f2d, 0x4b87, 0x89d4, 0x16eaedbe0001ULL)
#define BT_UUID_BME280_HUMIDITY_VAL \
    BT_UUID_128_ENCODE(0x9f06a2b3, 0x4f2d, 0x4b87, 0x89d4, 0x16eaedbe0002ULL)
#define BT_UUID_BME280_THRESHOLDS_VAL \
    BT_UUID_128_ENCODE(0x9f06a2b3, 0x4f2d, 0x4b87, 0x89d4, 0x16eaedbe0003ULL)

// UUID structs init
static struct bt_uuid_128 bme280_service_uuid = BT_UUID_INIT_128(BT_UUID_BME280_SERVICE_VAL);
static struct bt_uuid_128 temp_char_uuid = BT_UUID_INIT_128(BT_UUID_BME280_TEMP_VAL);
static struct bt_uuid_128 hum_char_uuid = BT_UUID_INIT_128(BT_UUID_BME280_HUMIDITY_VAL);
static struct bt_uuid_128 thresh_char_uuid = BT_UUID_INIT_128(BT_UUID_BME280_THRESHOLDS_VAL);

// Internal sensor values scaled by 100 for decimals
static int16_t temperature = 0;
static int16_t humidity = 0;

// Thresholds: temp_min, temp_max, hum_min, hum_max (also scaled)
static uint16_t thresholds[4] = {1500, 3000, 4000, 7000};

// Notification flags
static bool temp_notify_enabled = false;
static bool hum_notify_enabled = false;

// Current connection pointer (for notifications)
static struct bt_conn *current_conn = NULL;

// Read callbacks for characteristics
static ssize_t read_temp_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                            void *buf, uint16_t len, uint16_t offset)
{
    return bt_gatt_attr_read(conn, attr, buf, len, offset, &temperature, sizeof(temperature));
}

static ssize_t read_hum_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                           void *buf, uint16_t len, uint16_t offset)
{
    return bt_gatt_attr_read(conn, attr, buf, len, offset, &humidity, sizeof(humidity));
}

static ssize_t read_thresholds_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                                  void *buf, uint16_t len, uint16_t offset)
{
    return bt_gatt_attr_read(conn, attr, buf, len, offset, thresholds, sizeof(thresholds));
}

// Write callback for thresholds characteristic
static ssize_t write_thresholds_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                                   const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
    if (offset != 0 || len != sizeof(thresholds)) {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
    }
    memcpy(thresholds, buf, sizeof(thresholds));
    LOG_INF("Thresholds written via BLE");
    return len;
}

// CCCD changed callbacks
static void temp_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
    temp_notify_enabled = (value == BT_GATT_CCC_NOTIFY);
    LOG_WRN("Temperature notifications %s", temp_notify_enabled ? "enabled" : "disabled");
}

static void hum_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
    hum_notify_enabled = (value == BT_GATT_CCC_NOTIFY);
    LOG_WRN("Humidity notifications %s", hum_notify_enabled ? "enabled" : "disabled");
}

// Indexes of relevant attrs in bme280_svc.attrs
// Based on BT_GATT_SERVICE_DEFINE order:
// attrs[0] = primary service declaration
// attrs[1] = temp characteristic declaration
// attrs[2] = temp characteristic value <-- this is what we notify!
// attrs[3] = temp CCC descriptor
// attrs[4] = temp CUD descriptor
// attrs[5] = hum characteristic declaration
// attrs[6] = hum characteristic value <-- notify this
// attrs[7] = hum CCC descriptor
// attrs[8] = hum CUD descriptor
// attrs[9] = thresh characteristic declaration
// attrs[10] = thresh characteristic value (read/write)
// attrs[11] = thresh CUD descriptor

BT_GATT_SERVICE_DEFINE(bme280_svc,
    BT_GATT_PRIMARY_SERVICE(&bme280_service_uuid),

    BT_GATT_CHARACTERISTIC(&temp_char_uuid.uuid,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
                           BT_GATT_PERM_READ,
                           read_temp_cb, NULL, NULL),
    BT_GATT_CCC(temp_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
    BT_GATT_CUD("Temperature (°C)", BT_GATT_PERM_READ),

    BT_GATT_CHARACTERISTIC(&hum_char_uuid.uuid,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
                           BT_GATT_PERM_READ,
                           read_hum_cb, NULL, NULL),
    BT_GATT_CCC(hum_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
    BT_GATT_CUD("Humidity (%)", BT_GATT_PERM_READ),

    BT_GATT_CHARACTERISTIC(&thresh_char_uuid.uuid,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
                           BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
                           read_thresholds_cb, write_thresholds_cb, NULL),
    BT_GATT_CUD("Thresholds [t_min, t_max, h_min, h_max]", BT_GATT_PERM_READ),
);

// Init function (static registration, so só log)
void bme280_service_init(void)
{
    LOG_INF("BME280 BLE service initialized");
}

// Helper para notificar temperatura
void bme280_update_temperature(int16_t new_temperature)
{
    temperature = new_temperature;

    if (current_conn && temp_notify_enabled &&
        (temperature < (int16_t)thresholds[0] || temperature > (int16_t)thresholds[1])) {

        // Índice 2 corresponde ao valor da característica temp
        int err = bt_gatt_notify(current_conn, &bme280_svc.attrs[2], &temperature, sizeof(temperature));
        if (err == 0) {
            LOG_INF("Temperature notified: %d.%02d °C", temperature / 100, temperature % 100);
        } else {
            LOG_WRN("Failed to notify temperature, err: %d", err);
        }
    }
}

// Helper para notificar humidade
void bme280_update_humidity(int16_t new_humidity)
{
    humidity = new_humidity;

    if (current_conn && hum_notify_enabled &&
        (humidity < (int16_t)thresholds[2] || humidity > (int16_t)thresholds[3])) {

        // Índice 6 corresponde ao valor da característica hum
        int err = bt_gatt_notify(current_conn, &bme280_svc.attrs[6], &humidity, sizeof(humidity));
        if (err == 0) {
            LOG_INF("Humidity notified: %d.%02d %%", humidity / 100, humidity % 100);
        } else {
            LOG_WRN("Failed to notify humidity, err: %d", err);
        }
    }
}

// Atualizar thresholds manualmente
void bme280_update_thresholds(int16_t new_thresholds[4])
{
    memcpy(thresholds, new_thresholds, sizeof(thresholds));
}

// Set connection (ref/unref para evitar leaks)
void bme280_service_set_conn(struct bt_conn *conn)
{
    if (current_conn) {
        bt_conn_unref(current_conn);
        current_conn = NULL;
    }
    if (conn) {
        current_conn = bt_conn_ref(conn);
    }
}

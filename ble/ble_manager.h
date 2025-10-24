/*
 * Author: IlidioMSilva
 * Project: Sensor integration with Zephyr
 * Note: Educational/portfolio example
 */

#ifndef BLE_MANAGER_H_
#define BLE_MANAGER_H_

#include <zephyr/bluetooth/conn.h>

/**
 * @brief Initialize the BLE manager: Bluetooth stack, advertising, services, etc.
 *
 * @return 0 on success, negative error code on failure.
 */
int ble_manager_init(void);

/**
 * @brief Store or clear the active BLE connection reference.
 *
 * This is useful for other modules that may need to use the connection
 * (e.g., for notifications or tracking connection state).
 *
 * @param conn Pointer to the active connection, or NULL to clear.
 */
void ble_manager_set_connection(struct bt_conn *conn);

#endif /* BLE_MANAGER_H_ */

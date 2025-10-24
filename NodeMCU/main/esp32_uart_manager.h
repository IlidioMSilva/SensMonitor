/**
 * @brief UART manager for NodeMCU ESP32.
 *
 * Handles sending snapshot command (0x01) periodically to nRF52
 * and reading responses.
 */

#ifndef ESP32_UART_MANAGER_H
#define ESP32_UART_MANAGER_H

#include <Arduino.h>

#ifdef __cplusplus
extern "C" {
#endif

void uart_manager_init(void);
void uart_manager_update(void);

#ifdef __cplusplus
}
#endif

#endif /* ESP32_UART_MANAGER_H */

/**
 * @file    can_driver.h
 * @brief   FDCAN Driver (HAL Wrapper)
 * @author  Antigravity
 * @date    2026-01-31
 */

#ifndef MODULES_CAN_CAN_DRIVER_H_
#define MODULES_CAN_CAN_DRIVER_H_

#include "stm32g4xx_hal.h"
#include <stdbool.h>

typedef struct
{
    uint32_t id;
    uint8_t  len;
    uint8_t  data[8];
} CAN_Message_t;

/**
 * @brief Initialize FDCAN
 *        - Configures Filters
 *        - Starts FDCAN Module
 *        - Enables RX Interrupts
 * @param hfdcan FDCAN Handle (e.g., &hfdcan1)
 */
void CAN_Driver_Init(FDCAN_HandleTypeDef *hfdcan);

// Callback Type
typedef void (*CAN_RxCallback_t)(uint32_t id, uint8_t *data, uint8_t len);

// Register Rx Callback
void CAN_SetRxCallback(CAN_RxCallback_t callback);

/**
 * @brief Send a Standard CAN Frame
 * @param hfdcan FDCAN Handle
 * @param id Standard ID (11-bit)
 * @param data Data buffer (max 8 bytes)
 * @param len Data length
 * @return true if success
 */
bool CAN_Transmit(FDCAN_HandleTypeDef *hfdcan, uint32_t id, uint8_t *data, uint8_t len);

/**
 * @brief Check if a message was received (Simple flag check, or buffer availability)
 *        (Implementation depends on buffering strategy)
 */
bool CAN_IsMessageAvailable(void);

#endif /* MODULES_CAN_CAN_DRIVER_H_ */

/**
 * @file    imd_driver.h
 * @brief   Insulation Monitoring Device Driver (Bender iso165C Compatible)
 * @author  Antigravity
 * @date    2026-02-02
 */

#ifndef MODULES_IMD_DRIVER_H_
#define MODULES_IMD_DRIVER_H_

#include "main.h"
#include <stdbool.h>

// CAN IDs (Bender Default - Adjustable)
#define IMD_CAN_ID_TX_RESPONSE  0x23 // Example ID from IMD
#define IMD_CAN_ID_TX_INFO      0x24 // Periodic Info

typedef struct {
    float    insulation_resistance_kohm; // Measured R_iso
    bool     valid;             // Data validity
    bool     warning;           // Warning Threshold Reached
    bool     fault;             // Fault Threshold Reached
    uint32_t last_rx_tick;      // Timeout check
} IMD_Status_t;

/**
 * @brief Initialize IMD Driver
 * @param hfdcan Ptr to CAN handle (Shared with Power or SECC)
 */
void IMD_Init(FDCAN_HandleTypeDef *hfdcan);

/**
 * @brief Handle CAN Rx for IMD
 */
void IMD_RxHandler(uint32_t id, uint8_t *data, uint8_t len);

/**
 * @brief Get latest IMD Status
 */
const IMD_Status_t* IMD_GetStatus(void);

#endif /* MODULES_IMD_DRIVER_H_ */

/**
 * @file    secc_driver.h
 * @brief   SECC Communication Driver (EVerest-like BSP Protocol)
 * @author  Antigravity
 * @date    2026-01-31
 */

#ifndef MODULES_SECC_DRIVER_H_
#define MODULES_SECC_DRIVER_H_

#include "main.h"
#include <stdbool.h>

// CAN IDs
#define SECC_CAN_ID_TX_STATUS   0x600 // CCU -> SECC
#define SECC_CAN_ID_TX_METER    0x602 // CCU -> SECC (Meter Values)
#define SECC_CAN_ID_RX_CMD      0x610 // SECC -> CCU

// Control Data Structure
typedef struct {
    uint8_t target_pwm_duty; // 0-100% (Legacy AC)
    float   ev_target_voltage; // DC Target Voltage (V)
    float   ev_max_current;    // DC Max Current (A)
    uint8_t allow_power;     // 0=Open, 1=Close
    uint8_t reset_fault;     // 1=Trigger Reset
    bool    valid;           // True if fresh data received
    uint32_t last_rx_tick;   // For timeout
} SECC_Control_t;

extern SECC_Control_t secc_control;

/**
 * @brief Initialize SECC Driver (Register CAN Callback)
 * @param hfdcan Ptr to CAN handle
 */
void SECC_Init(FDCAN_HandleTypeDef *hfdcan);

/**
 * @brief Send Status Packet to SECC (Call every 50ms)
 * ...
 */
void SECC_TxStatus(float cp_volts_v, uint8_t pwm_duty, uint8_t relay_state, uint8_t err_code);

/**
 * @brief Send Meter Values (Call every 100-200ms)
 * @param ac_volts AC Voltage (V)
 * @param ac_amps AC Current (A)
 * @param temp_c Temperature (C)
 */
void SECC_TxMeter(float ac_volts, float ac_amps, float temp_c);

/**
 * @brief Handle CAN Rx Message (Called from CAN ISR/Callback)
 */
void SECC_RxHandler(uint32_t id, uint8_t *data, uint8_t len);

/**
 * @brief Check if SECC is connected (Heartbeat timeout)
 */
bool SECC_IsConnected(void);

#endif /* MODULES_SECC_DRIVER_H_ */

/**
 * @file    infy_power.h
 * @brief   Infypower Power Module (REG Series) Driver
 * @author  Antigravity
 *
 * @note    Reference Spec (Assumed Generic Rectifier Protocol):
 *          - Baudrate: 125 kbps (or system default)
 *          - Control (Broadcast): ID 0x18FFFF00 (Ext ID) or Standard 0x200
 *          - Status  (Unicast):   ID 0x18FFFF01 + NodeID
 *
 * @attention
 *          User requested sufficient comments for Power Module Control.
 *          This driver abstracts the CAN communication.
 */

#ifndef MODULES_POWER_INFY_POWER_H_
#define MODULES_POWER_INFY_POWER_H_

#include "main.h"
#include <stdbool.h>

// --- Configuration ---
#define INFY_USE_SIMULATION  1       // 1=Simulate Response, 0=Real CAN

// --- CAN Identifiers (Assumed) ---
// Control: PGN 0xFF00, Priority 6 -> 0x18FF00xx
#define INFY_CAN_ID_CONTROL  0x18005000 // Broadcast to all modules
#define INFY_CAN_ID_STATUS   0x18005001 // Base Response ID

// --- Data Structures ---

/**
 * @brief Power Module Status Structure
 */
typedef struct {
    float output_voltage;    // Measured Output Voltage (V)
    float output_current;    // Measured Output Current (A)
    bool  is_on;             // Output State (True=ON)
    bool  fault_ov;          // Over Voltage
    bool  fault_uv;          // Under Voltage
    bool  fault_ot;          // Over Temp
    bool  comm_timeout;      // Communication Lost
    uint32_t last_rx_tick;   // For timeout logic
} Infy_Status_t;

/**
 * @brief Initialize Power Module Driver
 * @param hfdcan Ptr to CAN handle
 */
void Infy_Init(FDCAN_HandleTypeDef *hfdcan);

/**
 * @brief Send Control Command to Power Module
 * @note  Call this periodically (e.g. 50ms or 100ms).
 *        Modules usually timeout if command is missing for >1s.
 *
 * @param target_volts  Target Voltage (V). Range: 150-1000V
 * @param target_amps   Target Current (A). Range: 0-100A
 * @param enable        Output Enable (True=ON, False=OFF/Safe)
 */
void Infy_SetOutput(float target_volts, float target_amps, bool enable);

/**
 * @brief Get Latest Status from Module
 * @return Information about V/I and Faults
 */
const Infy_Status_t* Infy_GetStatus(void);

/**
 * @brief Check if Power Module is Healthy
 * @return True if no faults and comms is OK
 */
bool Infy_IsHealthy(void);

#endif /* MODULES_POWER_INFY_POWER_H_ */

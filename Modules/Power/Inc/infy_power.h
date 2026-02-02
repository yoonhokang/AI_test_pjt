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
// --- Configuration ---
#define INFY_USE_SIMULATION  1       // 1=Simulate Response, 0=Real CAN
#define INFY_MAX_MODULES     10      // Max modules for 350kW+ (40kW * 10 = 400kW)

// --- CAN Identifiers (Assumed) ---
// Control: PGN 0xFF00, Priority 6 -> 0x18FF00xx
#define INFY_CAN_ID_CONTROL_BASE  0x18005000 // Broadcast to all modules
#define INFY_CAN_ID_STATUS_BASE   0x18005001 // Base Response ID (0x..01 ~ 0x..0A)

// --- Data Structures ---

/**
 * @brief Power Module Status Structure (Individual)
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
} Infy_ModuleStatus_t;

/**
 * @brief System Status Structure (Aggregated)
 */
typedef struct {
    float total_voltage;     // System Voltage (Max or Avg)
    float total_current;     // System Total Current (Sum)
    int   active_modules;    // Count of healthy modules
    bool  system_fault;      // Any module fault
} Infy_SystemStatus_t;

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
 * @brief Get Latest System Status (Aggregated)
 * @return Information about Total V/I
 */
const Infy_SystemStatus_t* Infy_GetSystemStatus(void);

/**
 * @brief Get Individual Module Status
 * @param index Module Index (0 ~ MAX-1)
 * @return Pointer to module status
 */
const Infy_ModuleStatus_t* Infy_GetModuleStatus(uint8_t index);

/**
 * @brief Check if Power System is Healthy
 * @return True if sufficient modules are active
 */
bool Infy_IsHealthy(void);

/**
 * @brief Handle Incoming CAN Messages
 * @param id CAN ID
 * @param data Payload
 * @param len Length
 */
void Infy_RxHandler(uint32_t id, uint8_t *data, uint8_t len);

#endif /* MODULES_POWER_INFY_POWER_H_ */

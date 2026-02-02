/**
 * @file    infy_power.c
 * @brief   Infypower Power Module Driver Implementation (Multi-Module Support)
 *
 * @details
 * This module handles the CAN communication with multiple Rectifier modules.
 * It implements load sharing (Current Distribution) and status aggregation.
 */

#include "infy_power.h"
#include "can_driver.h"
#include <stdio.h>
#include <stdlib.h> 
#include <string.h>

static FDCAN_HandleTypeDef *infy_hfdcan = NULL;

// Individual Module Status
static Infy_ModuleStatus_t modules[INFY_MAX_MODULES];

// System Aggregated Status
static Infy_SystemStatus_t system_status = {0};

// Simulation State (for Multi-Module)
static struct {
    float sim_volt;
    float sim_curr;
} sim_modules[INFY_MAX_MODULES];


void Infy_Init(FDCAN_HandleTypeDef *hfdcan)
{
    infy_hfdcan = hfdcan;
    
    // Clear Status
    memset(modules, 0, sizeof(modules));
    memset(&system_status, 0, sizeof(system_status));
    
    // Init Simulation
    for(int i=0; i<INFY_MAX_MODULES; i++) {
        modules[i].comm_timeout = true; 
        sim_modules[i].sim_volt = 0.0f;
    }

    printf("[Infy] Multi-Module Driver Initialized (%d Modules).\r\n", INFY_MAX_MODULES);
}

void Infy_RxHandler(uint32_t id, uint8_t *data, uint8_t len)
{
    // ID Range Check: 0x18005001 ~ 0x1800500A (Assuming base + 1..10)
    // Note: Adjust depending on actual Module ID configuration
    if (id <= INFY_CAN_ID_STATUS_BASE || id > (INFY_CAN_ID_STATUS_BASE + INFY_MAX_MODULES))
    {
        return;
    }

    int idx = id - INFY_CAN_ID_STATUS_BASE - 1; 
    if (idx < 0 || idx >= INFY_MAX_MODULES) return;

    // Parse Data (Assumed Format: V(2), I(2), State(1)...)
    // Byte 0-1: Voltage (0.1V)
    uint16_t v_raw = (data[0] << 8) | data[1];
    modules[idx].output_voltage = v_raw * 0.1f;

    // Byte 2-3: Current (0.1A)
    uint16_t i_raw = (data[2] << 8) | data[3];
    modules[idx].output_current = i_raw * 0.1f;

    // Byte 4: State/Faults
    modules[idx].is_on = (data[4] & 0x01) ? true : false;
    modules[idx].fault_ov = (data[4] & 0x02) ? true : false;
    // ... decode other faults

    modules[idx].last_rx_tick = HAL_GetTick();
    modules[idx].comm_timeout = false;
}


/**
 * @brief  Build and Send CAN Control Frame
 * @detail Packet Format (Assumed):
 *         Byte 0-1: Voltage (0.1V/bit)
 *         Byte 2-3: Current (0.1A/bit) - PER MODULE
 *         Byte 4:   Control (0x01=ON, 0x00=OFF)
 */
void Infy_SetOutput(float target_volts, float target_amps, bool enable)
{
    // --- 1. System Limits & Load Sharing ---
    float max_system_current = INFY_MAX_MODULES * 100.0f; // 100A per module
    if (target_amps > max_system_current) target_amps = max_system_current;
    if (target_volts > 1000.0f) target_volts = 1000.0f;

    // Count Active Modules for Distribution
    int active_count = 0;
    for(int i=0; i<INFY_MAX_MODULES; i++) {
        // Consider a module active if it's not timed out or we are just starting
        // For robustness, maybe assume all configured are active initially?
        // Simple logic: If enabled, assume we try to drive all.
        // Or better: Count healthy ones.
        if (!modules[i].comm_timeout) active_count++;
    }
    
    // Fail-safe: If no modules seen yet, assume all are available (at startup)
    if (active_count == 0 && enable) active_count = INFY_MAX_MODULES;
    if (active_count == 0) active_count = 1; // Avoid div by zero

    float current_per_module = target_amps / active_count;
    
    // Clamp per module
    if (current_per_module > 100.0f) current_per_module = 100.0f;

    if (!enable) {
        target_volts = 0.0f;
        current_per_module = 0.0f;
    }

#if INFY_USE_SIMULATION
    for(int i=0; i<INFY_MAX_MODULES; i++)
    {
        if (enable) {
             // Sim Ramp
             if (sim_modules[i].sim_volt < target_volts) sim_modules[i].sim_volt += 20.0f;
             else if (sim_modules[i].sim_volt > target_volts) sim_modules[i].sim_volt -= 20.0f;
             
             // Sim Load
             if (sim_modules[i].sim_volt > 300.0f) sim_modules[i].sim_curr = current_per_module;
             else sim_modules[i].sim_curr = 0.0f;
             
             modules[i].is_on = true;
        } else {
             sim_modules[i].sim_volt *= 0.9f;
             sim_modules[i].sim_curr = 0.0f;
             modules[i].is_on = false;
        }
        
        modules[i].output_voltage = sim_modules[i].sim_volt;
        modules[i].output_current = sim_modules[i].sim_curr;
        modules[i].comm_timeout = false;
        modules[i].last_rx_tick = HAL_GetTick(); // Keep alive
    }
    return;
#endif

    if (infy_hfdcan == NULL) return;

    // CAN Tx Logic (Broadcast)
    uint8_t data[8] = {0};
    uint16_t v_set = (uint16_t)(target_volts * 10.0f);
    uint16_t c_set = (uint16_t)(current_per_module * 10.0f); // Send Shared Current!
    
    data[0] = (uint8_t)(v_set >> 8);
    data[1] = (uint8_t)(v_set & 0xFF);
    data[2] = (uint8_t)(c_set >> 8);
    data[3] = (uint8_t)(c_set & 0xFF);
    data[4] = enable ? 0x01 : 0x00; 
    
    // Broadcast ID
    CAN_Transmit(infy_hfdcan, INFY_CAN_ID_CONTROL_BASE, data, 8);
}

const Infy_SystemStatus_t* Infy_GetSystemStatus(void)
{
    float max_v = 0.0f;
    float total_i = 0.0f;
    int valid_cnt = 0;
    bool any_fault = false;

    for (int i=0; i<INFY_MAX_MODULES; i++)
    {
        // Check Timeout (1s)
        if (HAL_GetTick() - modules[i].last_rx_tick > 1000) modules[i].comm_timeout = true;

        if (!modules[i].comm_timeout)
        {
            if (modules[i].output_voltage > max_v) max_v = modules[i].output_voltage;
            total_i += modules[i].output_current;
            valid_cnt++;
            
            if (modules[i].fault_ot || modules[i].fault_ov || modules[i].fault_uv) any_fault = true;
        }
    }
    
    system_status.active_modules = valid_cnt;
    system_status.total_voltage = max_v; // Use Max voltage for safety
    system_status.total_current = total_i;
    system_status.system_fault = any_fault;

    return &system_status;
}

const Infy_ModuleStatus_t* Infy_GetModuleStatus(uint8_t index)
{
    if (index >= INFY_MAX_MODULES) return NULL;
    return &modules[index];
}

bool Infy_IsHealthy(void)
{
    const Infy_SystemStatus_t *s = Infy_GetSystemStatus();
    // Logic: At least 1 module active and no Global Fault?
    // Or need minimum N modules? For now, simple check.
    if (s->system_fault) return false;
    if (s->active_modules == 0) return false; // No power available
    return true;
}

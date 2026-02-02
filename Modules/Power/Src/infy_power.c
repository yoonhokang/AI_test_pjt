/**
 * @file    infy_power.c
 * @brief   Infypower Power Module Driver Implementation
 *
 * @details
 * This module handles the low-level CAN communication with the Rectifier.
 * It abstracts the specific protocol (assumed Rectifier V1.0) into simple Set/Get APIs.
 *
 * ⚠️ Safety Note:
 * Always ensure 'enable' is FALSE when not actively charging.
 * Verify Output Voltage < 60V (Safe Voltage) before opening contactors.
 */

#include "infy_power.h"
#include <stdio.h>
#include <stdlib.h> // for rand()

static FDCAN_HandleTypeDef *infy_hfdcan = NULL;
static Infy_Status_t module_status = {0};

// Simulation State
static float sim_volt = 0.0f;
static float sim_curr = 0.0f;

void Infy_Init(FDCAN_HandleTypeDef *hfdcan)
{
    infy_hfdcan = hfdcan;
    module_status.comm_timeout = true; 
    printf("[Infy] Power Module Driver Initialized.\r\n");
}

/**
 * @brief  Build and Send CAN Control Frame
 * @detail Packet Format (Assumed):
 *         Byte 0-1: Voltage (0.1V/bit)
 *         Byte 2-3: Current (0.1A/bit)
 *         Byte 4:   Control (0x01=ON, 0x00=OFF)
 *         Byte 5-7: Resv
 */
void Infy_SetOutput(float target_volts, float target_amps, bool enable)
{
    // Safety Limit
    if (target_volts > 1000.0f) target_volts = 1000.0f;
    if (target_amps > 100.0f) target_amps = 100.0f;
    
    // Explicit Shutdown if disabled
    if (!enable)
    {
        target_volts = 0.0f; // Request 0V
        target_amps = 0.0f;  // Request 0A
    }

#if INFY_USE_SIMULATION
    // --- Simulation Logic ---
    // Simulate Voltage Ramp Up/Down (RC Filter response)
    if (enable)
    {
         // Ramp towards target (e.g., 50V per step)
         if (sim_volt < target_volts) sim_volt += 20.0f;
         else if (sim_volt > target_volts) sim_volt -= 20.0f;
         
         // Fix overshoot
         if (abs(sim_volt - target_volts) < 25.0f) sim_volt = target_volts;

         // Simulate Load: Current flows only if Voltage > 300V (Connected to EV)
         // Assuming Load Resistance approx 10 Ohm
         if (sim_volt > 300.0f) sim_curr = target_amps; // CC Mode
         else sim_curr = 0.0f;
         
         module_status.is_on = true;
    }
    else
    {
         // Discharge (Bleeder Resistor)
         if (sim_volt > 0) sim_volt *= 0.9f; 
         sim_curr = 0.0f;
         module_status.is_on = false;
    }
    
    // Update Mock Status
    module_status.output_voltage = sim_volt;
    module_status.output_current = sim_curr;
    module_status.comm_timeout = false;
    module_status.last_rx_tick = HAL_GetTick();
    
    // Don't actually send CAN in sim mode unless we want to test bus
    return;
#endif

    if (infy_hfdcan == NULL) return;

    // CAN Tx Logic
    uint8_t data[8] = {0};
    uint16_t v_set = (uint16_t)(target_volts * 10.0f);
    uint16_t c_set = (uint16_t)(target_amps * 10.0f);
    
    data[0] = (uint8_t)(v_set >> 8);
    data[1] = (uint8_t)(v_set & 0xFF);
    data[2] = (uint8_t)(c_set >> 8);
    data[3] = (uint8_t)(c_set & 0xFF);
    data[4] = enable ? 0x01 : 0x00; // Enable Command
    
    // Send Frame (Assuming helper exists or generic HAL)
    // CAN_Transmit(infy_hfdcan, INFY_CAN_ID_CONTROL, data, 8);
    (void)data; // Suppress unused warning
}

const Infy_Status_t* Infy_GetStatus(void)
{
    // Check Timeout
    if (HAL_GetTick() - module_status.last_rx_tick > 1000)
    {
        module_status.comm_timeout = true;
    }
    return &module_status;
}

bool Infy_IsHealthy(void)
{
    const Infy_Status_t *s = Infy_GetStatus();
    if (s->comm_timeout) return false;
    if (s->fault_ot || s->fault_ov || s->fault_uv) return false;
    return true;
}

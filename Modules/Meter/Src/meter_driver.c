/**
 * @file    meter_driver.c
 * @brief   Power Meter Driver Implementation
 */

#include "meter_driver.h"
#include <math.h>
#include <stdlib.h>

// Internal State for Simulation
static float sim_voltage = 220.0f;
static float sim_current = 0.0f;
static float sim_temp = 25.0f;

void Meter_Init(void)
{
    // Need to init ADC if Real Mode
    // HAL_ADC_Start(&hadc1); 
}

float Meter_ReadVoltage(void)
{
    #if METER_SIMULATION_MODE
        // Simulate slight fluctuation
        float noise = ((rand() % 100) - 50) / 100.0f; // -0.5 to +0.5
        return sim_voltage + noise;
    #else
        // Implement Real ADC Reading
        // return ADC_GetVoltage();
        return 0.0f;
    #endif
}

float Meter_ReadCurrent(void)
{
    #if METER_SIMULATION_MODE
        // Depends on State? 
        // We can link this to Relay State later.
        // For now, return stored sim value
        return sim_current;
    #else
        // Implement Real ADC Reading
        return 0.0f;
    #endif
}

float Meter_ReadTemperature(void)
{
    #if METER_SIMULATION_MODE
        // Slowly increase if current > 0
        if (sim_current > 1.0f) sim_temp += 0.01f;
        else if (sim_temp > 25.0f) sim_temp -= 0.01f;
        
        return sim_temp;
    #else
        return 25.0f;
    #endif
}

float Meter_ReadPower(void)
{
    return Meter_ReadVoltage() * Meter_ReadCurrent();
}

// Helper to set Simulation Data (called from App/CLI)
void Meter_Sim_SetCurrent(float amps)
{
    sim_current = amps;
}

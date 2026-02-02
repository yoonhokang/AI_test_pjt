/**
 * @file    safety_monitor.c
 * @brief   Safety Monitor Implementation
 */

#include "safety_monitor.h"
#include "imd_driver.h"
#include "meter_driver.h"
#include <stdio.h>

void Safety_Init(void)
{
    printf("[Safety] Monitor Initialized (E-Stop, IMD, Temp).\r\n");
    // Initial Check
    Safety_Check();
}

Safety_Status_t Safety_Check(void)
{
    // 1. E-Stop Check (Hardwired)
    GPIO_PinState estop_state = HAL_GPIO_ReadPin(Emergency_Stop_GPIO_Port, Emergency_Stop_Pin);
    if (estop_state == GPIO_PIN_RESET) 
    {
        return SAFETY_FAULT_ESTOP;
    }

    // 2. IMD Check (Modbus/CAN)
    const IMD_Status_t *imd = IMD_GetStatus();
    if (imd->valid)
    {
        if (imd->fault || imd->insulation_resistance_kohm < 100.0f) // Threshold: 100 kOhm
        {
            printf("[Safety] IMD Fault! R_iso: %.1f kOhm\r\n", imd->insulation_resistance_kohm);
            return SAFETY_FAULT_IMD;
        }
    }
    else
    {
        // Require IMD Communication for High Power?
        // return SAFETY_FAULT_IMD; // Strict Safety
    }

    // 3. Over-Temperature Check
    float temp = Meter_ReadTemperature();
    if (temp > 85.0f)
    {
        printf("[Safety] Over Temperature! %.1f C\r\n", temp);
        return SAFETY_FAULT_OVERTEMP;
    }

    return SAFETY_OK;
}

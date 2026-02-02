/**
 * @file    relay_driver.c
 * @brief   Relay Driver Implementation
 */

#include "relay_driver.h"
#include <stdio.h>

// Configuration: Assume Active HIGH (High = Relay Closed/On)
#define RELAY_ACTIVE_STATE GPIO_PIN_SET
#define RELAY_SAFE_STATE   GPIO_PIN_RESET

void Relay_Init(void)
{
    // Ensure all relays are Open on init
    Relay_SetMain(false);
    Relay_SetPrecharge(false);
    
    printf("[Relay] Initialized (Safe State: Open)\r\n");
}

void Relay_SetMain(bool closed)
{
    GPIO_PinState state = closed ? RELAY_ACTIVE_STATE : RELAY_SAFE_STATE;
    
    // Control both P and N contactors simultaneously
    HAL_GPIO_WritePin(Relay_Main_P_GPIO_Port, Relay_Main_P_Pin, state);
    HAL_GPIO_WritePin(Relay_Main_N_GPIO_Port, Relay_Main_N_Pin, state);
    
    // printf/Log can be added here if needed, but might spam if called frequently
}

void Relay_SetPrecharge(bool closed)
{
    // [Hardware Config] No Physical Pre-charge Relay exists.
    // Logic is handled by Power Module Soft-Start (Voltage Matching).
    // Just log the intent for debugging.
    if (closed)
    {
        printf("[Relay] Pre-charge Virtual State: CLOSED (Soft-Start Logic Active)\r\n");
    }
    else
    {
        printf("[Relay] Pre-charge Virtual State: OPEN\r\n");
    }
    
    // GPIO_PinState state = closed ? RELAY_ACTIVE_STATE : RELAY_SAFE_STATE;
    // HAL_GPIO_WritePin(Relay_Precharge_GPIO_Port, Relay_Precharge_Pin, state);
}

uint8_t Relay_GetState(void)
{
    uint8_t state = 0;
    if (HAL_GPIO_ReadPin(Relay_Main_P_GPIO_Port, Relay_Main_P_Pin) == RELAY_ACTIVE_STATE)
    {
        state |= 0x01;
    }
    if (HAL_GPIO_ReadPin(Relay_Precharge_GPIO_Port, Relay_Precharge_Pin) == RELAY_ACTIVE_STATE)
    {
        state |= 0x02;
    }
    return state;
}

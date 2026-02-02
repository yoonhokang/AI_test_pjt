/**
 * @file    watchdog_driver.c
 * @brief   Watchdog Driver Implementation
 */

#include "watchdog_driver.h"
#include <stdio.h>

#if USE_HARDWARE_IWDG
extern IWDG_HandleTypeDef hiwdg;
#endif

void Watchdog_Init(void)
{
    #if USE_HARDWARE_IWDG
    // Check if IWDG was initialized by HAL
    // if (hiwdg.Instance == NULL) ...
    printf("[WDT] Hardware Watchdog Active\r\n");
    #else
    printf("[WDT] Simulation Mode (No hardware reset)\r\n");
    #endif
}

void Watchdog_Refresh(void)
{
    #if USE_HARDWARE_IWDG
    if (HAL_IWDG_Refresh(&hiwdg) != HAL_OK)
    {
        // Error handling?
    }
    #else
    // Simulating Feed
    // static uint32_t last_feed = 0;
    // if (HAL_GetTick() - last_feed > 5000) printf("[WDT] *Fed*\r\n");
    // last_feed = HAL_GetTick();
    #endif
}

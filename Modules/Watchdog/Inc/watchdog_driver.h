/**
 * @file    watchdog_driver.h
 * @brief   Independent Watchdog (IWDG) Driver Wrapper
 */

#ifndef MODULES_WATCHDOG_WATCHDOG_DRIVER_H_
#define MODULES_WATCHDOG_WATCHDOG_DRIVER_H_

#include "main.h"

// Set to 0 if IWDG is not enabled in CubeMX/Hardware
#define USE_HARDWARE_IWDG   0 

/**
 * @brief Initialize Watchdog (if needed, usually done by CubeMX)
 */
void Watchdog_Init(void);

/**
 * @brief Refresh the Watchdog Timer.
 * Call this periodically within the timeout window (e.g. 1s).
 */
void Watchdog_Refresh(void);

#endif /* MODULES_WATCHDOG_WATCHDOG_DRIVER_H_ */

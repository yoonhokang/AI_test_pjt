/**
 * @file    safety_monitor.h
 * @brief   Safety Monitoring Driver
 * @author  Antigravity
 * @date    2026-01-31
 */

#ifndef MODULES_SAFETY_MONITOR_H_
#define MODULES_SAFETY_MONITOR_H_

#include "main.h"
#include <stdbool.h>

typedef enum
{
    SAFETY_OK = 0,
    SAFETY_FAULT_ESTOP,
    SAFETY_FAULT_OVERTEMP, // Future placeholder
    SAFETY_FAULT_GFCI      // Future placeholder
} Safety_Status_t;

/**
 * @brief Initialize Safety Monitor (GPIOs etc)
 */
void Safety_Init(void);

/**
 * @brief Check all safety conditions
 * @return SAFETY_OK if safe, otherwise Error Code
 */
Safety_Status_t Safety_Check(void);

#endif /* MODULES_SAFETY_MONITOR_H_ */

/**
 * @file    relay_driver.h
 * @brief   High Voltage Relay Control Driver
 * @author  Antigravity
 * @date    2026-01-31
 */

#ifndef MODULES_RELAY_RELAY_DRIVER_H_
#define MODULES_RELAY_RELAY_DRIVER_H_

#include "main.h"
#include <stdbool.h>

/**
 * @brief Initialize Relay GPIOs to Safe State (Open)
 */
void Relay_Init(void);

/**
 * @brief Control Main Contactor (Positive & Negative)
 * @param closed true=Close (Power ON), false=Open (Safe)
 */
void Relay_SetMain(bool closed);

/**
 * @brief Control Precharge Relay
 * @param closed true=Close (Resistor Active), false=Open
 */
void Relay_SetPrecharge(bool closed);

/**
 * @brief Get Relay Status
 * @return Bit0: Main, Bit1: Precharge
 */
uint8_t Relay_GetState(void);

#endif /* MODULES_RELAY_RELAY_DRIVER_H_ */

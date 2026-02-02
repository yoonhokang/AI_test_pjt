/**
 * @file    config_manager.h
 * @brief   System Configuration Storage Manager
 */

#ifndef MODULES_COMMON_CONFIG_MANAGER_H_
#define MODULES_COMMON_CONFIG_MANAGER_H_

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t magic_code;      // 0xA5A5A5A5 if valid
    float    max_current_a;   // Limit
    uint8_t  last_fault_code; // Persistent Fault
    uint32_t boot_count;      // Diagnostics
} SystemConfig_t;

/**
 * @brief Initialize Config (Load from Flash)
 */
void Config_Init(void);

/**
 * @brief Get Pointer to System Config
 */
SystemConfig_t* Config_Get(void);

/**
 * @brief Save Current Config to Flash
 */
void Config_Save(void);

/**
 * @brief Reset to Defaults
 */
void Config_ResetDefaults(void);

#endif /* MODULES_COMMON_CONFIG_MANAGER_H_ */

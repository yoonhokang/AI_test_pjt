/**
 * @file    config_manager.c
 * @brief   Config Manager Implementation
 */

#include "config_manager.h"
#include <stdio.h>
#include <string.h>

#define CONFIG_MAGIC 0xA5A5A5A5

static SystemConfig_t sys_config;

void Config_ResetDefaults(void)
{
    sys_config.magic_code = CONFIG_MAGIC;
    sys_config.max_current_a = 32.0f;
    sys_config.last_fault_code = 0;
    sys_config.boot_count = 0;
    
    printf("[Config] Reset to Defaults.\r\n");
}

void Config_Init(void)
{
    // Simulate Load from Flash Address
    // memcpy(&sys_config, (void*)0x0807F800, sizeof(SystemConfig_t));
    
    // Check Validity (Mock)
    if (sys_config.magic_code != CONFIG_MAGIC)
    {
        printf("[Config] Invalid/Empty Flash. Loading Defaults.\r\n");
        Config_ResetDefaults();
    }
    else
    {
        printf("[Config] Loaded. Boot Count: %lu\r\n", sys_config.boot_count);
        sys_config.boot_count++;
    }
}

SystemConfig_t* Config_Get(void)
{
    return &sys_config;
}

void Config_Save(void)
{
    // Simulate Flash Erase/Write
    printf("[Config] Saving to Flash... ");
    // HAL_FLASH_Unlock();
    // ...
    // HAL_FLASH_Lock();
    printf("Done. (Mock)\r\n");
}

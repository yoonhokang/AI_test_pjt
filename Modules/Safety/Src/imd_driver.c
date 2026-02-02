/**
 * @file    imd_driver.c
 * @brief   IMD Driver Implementation
 */

#include "imd_driver.h"
#include <stdio.h>

static FDCAN_HandleTypeDef *imd_hfdcan = NULL;
static IMD_Status_t imd_status = {0};

void IMD_Init(FDCAN_HandleTypeDef *hfdcan)
{
    imd_hfdcan = hfdcan;
    imd_status.insulation_resistance_kohm = 9999.9f; // Init as High Z
    imd_status.valid = false;
    imd_status.warning = false;
    imd_status.fault = false;
    
    printf("[IMD] Initialized (CAN Mode).\r\n");
}

void IMD_RxHandler(uint32_t id, uint8_t *data, uint8_t len)
{
    // Simplified parsing for Bender-like protocol
    // Usually standard format: Byte 0-1 = Resistance, Byte 2 = Flags
    
    if (id == IMD_CAN_ID_TX_INFO && len >= 4)
    {
        // Example Mapping (Big Endian)
        uint16_t r_iso_raw = (data[0] << 8) | data[1];
        imd_status.insulation_resistance_kohm = (float)r_iso_raw; // 1 kOhm/bit or similar
        
        uint8_t flags = data[2];
        imd_status.warning = (flags & 0x01) ? true : false;
        imd_status.fault   = (flags & 0x02) ? true : false;
        
        imd_status.last_rx_tick = HAL_GetTick();
        imd_status.valid = true;
    }
}

const IMD_Status_t* IMD_GetStatus(void)
{
    // Check timeout (e.g. 500ms)
    if (HAL_GetTick() - imd_status.last_rx_tick > 1000)
    {
        imd_status.valid = false;
    }
    return &imd_status;
}

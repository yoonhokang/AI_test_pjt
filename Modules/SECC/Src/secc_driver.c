/**
 * @file    secc_driver.c
 * @brief   SECC Driver Implementation
 */

#include "secc_driver.h"
#include "can_driver.h"
#include <stdio.h>
#include <string.h>

static FDCAN_HandleTypeDef *secc_hfdcan = NULL;
SECC_Control_t secc_control = {0};

void SECC_Init(FDCAN_HandleTypeDef *hfdcan)
{
    secc_hfdcan = hfdcan;
    secc_control.valid = false;
    secc_control.last_rx_tick = 0;
    
    printf("[SECC] Initialized. Waiting for 0x610...\r\n");
}

void SECC_TxStatus(float cp_volts_v, uint8_t pwm_duty, uint8_t relay_state, uint8_t err_code)
{
    if (secc_hfdcan == NULL) return;

    uint8_t data[8] = {0};
    
    // Byte 0-1: CP Voltage (mV)
    uint16_t cp_mv = (uint16_t)(cp_volts_v * 1000.0f);
    data[0] = cp_mv & 0xFF;
    data[1] = (cp_mv >> 8) & 0xFF;
    
    // Byte 2-3: PP Voltage (mV) - Placeholder 0
    data[2] = 0;
    data[3] = 0;
    
    // Byte 4: PWM Duty
    data[4] = pwm_duty;
    
    // Byte 5: Relay State
    data[5] = relay_state;
    
    // Byte 6: Error Code
    data[6] = err_code;
    
    // Byte 7: Reserved
    data[7] = 0;
    
    CAN_Transmit(secc_hfdcan, SECC_CAN_ID_TX_STATUS, data, 8);
}

void SECC_TxMeter(float ac_volts, float ac_amps, float temp_c)
{
    if (secc_hfdcan == NULL) return;

    uint8_t data[8] = {0};
    
    // V (10x)
    uint16_t v_scale = (uint16_t)(ac_volts * 10.0f);
    data[0] = v_scale & 0xFF;
    data[1] = (v_scale >> 8) & 0xFF;
    
    // I (10x)
    uint16_t i_scale = (uint16_t)(ac_amps * 10.0f);
    data[2] = i_scale & 0xFF;
    data[3] = (i_scale >> 8) & 0xFF;
    
    // Temp (1x, offset?) Assuming -40 to 215. int8
    data[4] = (int8_t)temp_c;
    
    // Checksum simulation? No.
    
    CAN_Transmit(secc_hfdcan, SECC_CAN_ID_TX_METER, data, 8);
}

void SECC_RxHandler(uint32_t id, uint8_t *data, uint8_t len)
{
    if (id == SECC_CAN_ID_RX_CMD && len >= 3)
    {
        secc_control.target_pwm_duty = data[0];
        secc_control.allow_power = data[1];
        secc_control.reset_fault = data[2];
        
        secc_control.last_rx_tick = HAL_GetTick();
        secc_control.valid = true;
        
        // Debug Log (Throttled?)
        // printf("[SECC] Rx Cmd: PWM=%d Allow=%d Res=%d\r\n", data[0], data[1], data[2]);
    }
}

bool SECC_IsConnected(void)
{
    if (!secc_control.valid) return false;
    
    // 3 Second Timeout
    if ((HAL_GetTick() - secc_control.last_rx_tick) > 3000)
    {
        return false;
    }
    return true;
}

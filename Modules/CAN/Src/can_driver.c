/**
 * @file    can_driver.c
 * @brief   FDCAN Driver Implementation
 */

#include "can_driver.h"
#include <stdio.h>
#include <string.h>

static FDCAN_HandleTypeDef *drv_hfdcan = NULL;
static CAN_RxCallback_t rx_callback = NULL;

void CAN_Driver_Init(FDCAN_HandleTypeDef *hfdcan)
{
    if (hfdcan == NULL) return;
    drv_hfdcan = hfdcan;

    // 1. Configure Filter (Accept All Standard Frames)
    FDCAN_FilterTypeDef sFilterConfig;
    sFilterConfig.IdType = FDCAN_STANDARD_ID;
    sFilterConfig.FilterIndex = 0;
    sFilterConfig.FilterType = FDCAN_FILTER_MASK;
    sFilterConfig.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
    sFilterConfig.FilterID1 = 0x000;
    sFilterConfig.FilterID2 = 0x000; // Mask 0x000 = Accept All

    if (HAL_FDCAN_ConfigFilter(hfdcan, &sFilterConfig) != HAL_OK)
    {
        printf("[CAN] Filter Config Error\r\n");
    }

    // 2. Start FDCAN
    if (HAL_FDCAN_Start(hfdcan) != HAL_OK)
    {
        printf("[CAN] Start Error\r\n");
    }

    // 3. Activate Notification (RX FIFO 0 New Message)
    if (HAL_FDCAN_ActivateNotification(hfdcan, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0) != HAL_OK)
    {
        printf("[CAN] Notification Error\r\n");
    }

    printf("[CAN] Initialized (Rate: 500k/2MB)\r\n");
}

void CAN_SetRxCallback(CAN_RxCallback_t callback)
{
    rx_callback = callback;
}

bool CAN_Transmit(FDCAN_HandleTypeDef *hfdcan, uint32_t id, uint8_t *data, uint8_t len)
{
    FDCAN_TxHeaderTypeDef TxHeader;

    TxHeader.Identifier = id;
    TxHeader.IdType = FDCAN_STANDARD_ID;
    TxHeader.TxFrameType = FDCAN_DATA_FRAME;
    
    switch(len) {
        case 0: TxHeader.DataLength = FDCAN_DLC_BYTES_0; break;
        case 1: TxHeader.DataLength = FDCAN_DLC_BYTES_1; break;
        case 2: TxHeader.DataLength = FDCAN_DLC_BYTES_2; break;
        case 3: TxHeader.DataLength = FDCAN_DLC_BYTES_3; break;
        case 4: TxHeader.DataLength = FDCAN_DLC_BYTES_4; break;
        case 5: TxHeader.DataLength = FDCAN_DLC_BYTES_5; break;
        case 6: TxHeader.DataLength = FDCAN_DLC_BYTES_6; break;
        case 7: TxHeader.DataLength = FDCAN_DLC_BYTES_7; break;
        case 8: TxHeader.DataLength = FDCAN_DLC_BYTES_8; break;
        default: TxHeader.DataLength = FDCAN_DLC_BYTES_8; break;
    }

    TxHeader.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    TxHeader.BitRateSwitch = FDCAN_BRS_OFF; // Classic CAN
    TxHeader.FDFormat = FDCAN_CLASSIC_CAN;
    TxHeader.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
    TxHeader.MessageMarker = 0;

    if (HAL_FDCAN_AddMessageToTxFifoQ(hfdcan, &TxHeader, data) != HAL_OK)
    {
        return false;
    }
    return true;
}

void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs)
{
    if((RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) != RESET)
    {
        FDCAN_RxHeaderTypeDef RxHeader;
        uint8_t RxData[8];

        if (HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO0, &RxHeader, RxData) == HAL_OK)
        {
            if (rx_callback != NULL)
            {
                rx_callback(RxHeader.Identifier, RxData, 8); // Assume 8 or calculate real len
            }
        }
    }
}

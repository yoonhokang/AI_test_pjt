/**
 * @file    ocpp_app.h
 * @brief   OCPP 1.6J Client Application
 */

#ifndef MODULES_OCPP_OCPP_APP_H_
#define MODULES_OCPP_OCPP_APP_H_

#include "main.h"

typedef enum {
    OCPP_STATE_OFFLINE,
    OCPP_STATE_CONNECTING,
    OCPP_STATE_BOOTING,      // Sending BootNotification
    OCPP_STATE_CHARGING,     // Transacting
    OCPP_STATE_IDLE
} OCPP_State_t;

/**
 * @brief Initialize OCPP Task
 */
void OCPP_Init(void);

/**
 * @brief Periodic Process (Call in FreeRTOS Task or Main Loop)
 */
void OCPP_Process(void);

/**
 * @brief Send StartTransaction
 */
void OCPP_SendStartTransaction(const char* id_tag);

/**
 * @brief Send StopTransaction
 */
void OCPP_SendStopTransaction(void);

#endif /* MODULES_OCPP_OCPP_APP_H_ */

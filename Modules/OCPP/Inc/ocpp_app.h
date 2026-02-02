/**
 * @file    ocpp_app.h
 * @brief   OCPP 1.6J Client Application
 */

#ifndef MODULES_OCPP_OCPP_APP_H_
#define MODULES_OCPP_OCPP_APP_H_

#include "main.h"

typedef enum {
    OCPP_STATE_OFFLINE,
    OCPP_STATE_TCP_CONNECTING, // Waiting for TCP SYN+ACK
    OCPP_STATE_TLS_HANDSHAKE,  // Doing TLS Handshake
    OCPP_STATE_CONNECTING,     // Sending WS Upgrade
    OCPP_STATE_BOOTING,        // Sending BootNotification
    OCPP_STATE_CHARGING,       // Transacting
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

/**
 * @brief Send Status Notification
 * @param connectorId Connector ID (1-based)
 * @param status Status key (e.g. "Available", "Charging")
 * @param error_code Error code (e.g. "NoError")
 */
void OCPP_SendStatusNotification(int connectorId, const char* status, const char* error_code);

/**
 * @brief Send Meter Values
 * @param connectorId Connector ID
 * @param power_w Power in Watts
 * @param energy_wh Energy in Wh
 * @param soc SoC %
 */
void OCPP_SendMeterValues(int connectorId, float power_w, float energy_wh, int soc);

#endif /* MODULES_OCPP_OCPP_APP_H_ */

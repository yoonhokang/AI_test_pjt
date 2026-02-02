/**
 * @file    ocpp_app.c
 * @brief   OCPP Client Implementation (TLS Enabled)
 */

#include "ocpp_app.h"
#include "w5500_driver.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include <stdio.h>
#include <string.h>

// External Port Functions
extern int mbedtls_net_send(void *ctx, const unsigned char *buf, size_t len);
extern int mbedtls_net_recv(void *ctx, unsigned char *buf, size_t len);
extern int mbedtls_hardware_poll(void *data, unsigned char *output, size_t len, size_t *olen);

// MbedTLS Contexts
static mbedtls_ssl_context ssl;
static mbedtls_ssl_config conf;
static mbedtls_ctr_drbg_context ctr_drbg;
static mbedtls_entropy_context entropy;

static OCPP_State_t ocpp_state = OCPP_STATE_OFFLINE;
static uint32_t ocpp_tick = 0;

void OCPP_Init(void)
{
    // Initialize Ethernet
    W5500_Init();
    
    // Initialize TLS
    mbedtls_ssl_init(&ssl);
    mbedtls_ssl_config_init(&conf);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_entropy_init(&entropy);
    
    // Seed RNG
    mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, (const unsigned char*)"EVSE", 4);
    
    // Config Defaults
    mbedtls_ssl_config_defaults(&conf, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT);
    mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);
    mbedtls_ssl_setup(&ssl, &conf);
    
    ocpp_state = OCPP_STATE_OFFLINE;
    printf("[OCPP] Initialized (TLS Enabled).\r\n");
}

void OCPP_Process(void)
{
    switch (ocpp_state)
    {
        case OCPP_STATE_OFFLINE:
            if (HAL_GetTick() - ocpp_tick > 5000) // Retry every 5s
            {
                ocpp_tick = HAL_GetTick();
                
                // Try Connect (TCP)
                uint8_t server_ip[4] = {192, 168, 0, 100}; 
                if (W5500_Socket(0, SN_MR_TCP, 2020))
                {
                    if (W5500_Connect(0, server_ip, 8080))
                    {
                        printf("[OCPP] TCP Connected. Starting TLS Handshake...\r\n");
                        // Bind IO
                        mbedtls_ssl_set_bio(&ssl, (void*)0, mbedtls_net_send, mbedtls_net_recv);
                        
                        // Handshake
                        int ret = mbedtls_ssl_handshake(&ssl);
                        if (ret == 0)
                        {
                            printf("[OCPP] TLS Handshake Success.\r\n");
                            ocpp_state = OCPP_STATE_CONNECTING;
                        }
                        else
                        {
                            printf("[OCPP] TLS Handshake Failed: -0x%x\r\n", -ret);
                            W5500_Close(0);
                        }
                    }
                }
            }
            break;
            
        case OCPP_STATE_CONNECTING:
            // Send WebSocket Upgrade (Simulated HTTP)
            printf("[OCPP] Sending WS Handshake (TLS)...\r\n");
            char ws_req[] = "GET /ocpp/CP1 HTTP/1.1\r\nUpgrade: websocket\r\n\r\n";
            mbedtls_ssl_write(&ssl, (unsigned char*)ws_req, strlen(ws_req));
            
            ocpp_state = OCPP_STATE_BOOTING;
            ocpp_tick = HAL_GetTick();
            break;
            
        case OCPP_STATE_BOOTING:
            if (HAL_GetTick() - ocpp_tick > 1000)
            {
                printf("[OCPP] Sending BootNotification...\r\n");
                char boot_json[] = "[2, \"1001\", \"BootNotification\", {\"vendor\": \"TestFw\"}]";
                mbedtls_ssl_write(&ssl, (unsigned char*)boot_json, strlen(boot_json));
                
                ocpp_state = OCPP_STATE_IDLE;
            }
            break;
            
        case OCPP_STATE_IDLE:
        case OCPP_STATE_CHARGING:
            // Read Loop (if any)
            // unsigned char buf[128];
            // int len = mbedtls_ssl_read(&ssl, buf, sizeof(buf));
            break;
            
        default: break;
    }
}

void OCPP_SendStartTransaction(const char* id_tag)
{
    if (ocpp_state != OCPP_STATE_IDLE) return;
    
    char buf[128];
    snprintf(buf, sizeof(buf), "[2, \"1002\", \"StartTransaction\", {\"tag\": \"%s\"}]", id_tag);
    printf("[OCPP] Tx Start: %s\r\n", buf);
    mbedtls_ssl_write(&ssl, (unsigned char*)buf, strlen(buf));
    
    ocpp_state = OCPP_STATE_CHARGING;
}

void OCPP_SendStopTransaction(void)
{
    if (ocpp_state != OCPP_STATE_CHARGING) return;
    
    char buf[128];
    snprintf(buf, sizeof(buf), "[2, \"1003\", \"StopTransaction\", {\"id\": 1}]");
    printf("[OCPP] Tx Stop: %s\r\n", buf);
    mbedtls_ssl_write(&ssl, (unsigned char*)buf, strlen(buf));
    
    ocpp_state = OCPP_STATE_IDLE;
}

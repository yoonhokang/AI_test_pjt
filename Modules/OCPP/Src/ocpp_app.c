/**
 * @file    ocpp_app.c
 * @brief   OCPP Client Implementation (TLS Enabled)
 */

#include "ocpp_app.h"
#include "app_state.h" // For StateMachine control
#define JSMN_IMPLEMENTATION
#include "jsmn.h"      // For JSON Parsing
#include "w5500_driver.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include <stdio.h>
#include <stdio.h>
#include <string.h>
#include "config_manager.h" // For SystemConfig

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

// Rx Handler Prototypes
static void Handle_RemoteStartTransaction(jsmntok_t *tokens, int num_tokens, const char *json);
static void Handle_RemoteStopTransaction(jsmntok_t *tokens, int num_tokens, const char *json);
static void Handle_CallMessage(const char* json, size_t len);

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
    unsigned char buf[1024]; // Rx Buffer
    int len;

    switch (ocpp_state)
    {
        case OCPP_STATE_OFFLINE:
            if (HAL_GetTick() - ocpp_tick > 5000) // Retry every 5s
            {
                ocpp_tick = HAL_GetTick();
                
                // Try Connect Start (Async)
                SystemConfig_t *cfg = Config_Get();
                // uint8_t server_ip[4] = {192, 168, 0, 100}; 
                if (W5500_Socket(0, SN_MR_TCP, 2020))
                {
                    if (W5500_Connect_Start(0, cfg->server_ip, cfg->server_port))
                    {
                        ocpp_state = OCPP_STATE_TCP_CONNECTING;
                    }
                }
            }
            break;

        case OCPP_STATE_TCP_CONNECTING:
            {
                 // Check Timeout (e.g. 3s)
                 if (HAL_GetTick() - ocpp_tick > 3000)
                 {
                     printf("[OCPP] TCP Connect Timeout.\r\n");
                     W5500_Close(0);
                     ocpp_state = OCPP_STATE_OFFLINE;
                     break;
                 }

                 uint8_t sr = W5500_Connect_Poll(0);
                 if (sr == SOCK_ESTABLISHED)
                 {
                     printf("[OCPP] TCP Connected. Starting TLS Handshake...\r\n");
                     // Bind IO
                     mbedtls_ssl_set_bio(&ssl, (void*)0, mbedtls_net_send, mbedtls_net_recv);
                     
                     ocpp_state = OCPP_STATE_TLS_HANDSHAKE;
                 }
                 else if (sr == SOCK_CLOSED)
                 {
                     printf("[OCPP] TCP Connect Failed (Closed).\r\n");
                     ocpp_state = OCPP_STATE_OFFLINE;
                 }
            }
            break;

        case OCPP_STATE_TLS_HANDSHAKE:
            {
                int ret = mbedtls_ssl_handshake(&ssl);
                if (ret == 0)
                {
                    printf("[OCPP] TLS Handshake Success.\r\n");
                    ocpp_state = OCPP_STATE_CONNECTING;
                }
                else if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE)
                {
                    printf("[OCPP] TLS Handshake Failed: -0x%x\r\n", -ret);
                    W5500_Close(0);
                    ocpp_state = OCPP_STATE_OFFLINE;
                }
                // If WANT_READ/WRITE, stay in this state
            }
            break;
            
        case OCPP_STATE_CONNECTING:
            // Send WebSocket Upgrade (Simulated HTTP)
            printf("[OCPP] Sending WS Handshake (TLS)...\r\n");
            char ws_req[256];
            SystemConfig_t *cfg = Config_Get();
            snprintf(ws_req, sizeof(ws_req), 
                     "GET /ocpp/%s HTTP/1.1\r\n"
                     "Host: %d.%d.%d.%d:%d\r\n"
                     "Upgrade: websocket\r\n"
                     "Connection: Upgrade\r\n"
                     "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
                     "Sec-WebSocket-Version: 13\r\n"
                     "Sec-WebSocket-Protocol: ocpp1.6\r\n\r\n",
                     cfg->charge_box_id,
                     cfg->server_ip[0], cfg->server_ip[1], cfg->server_ip[2], cfg->server_ip[3], cfg->server_port);
                     
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
            // RX Loop
            len = mbedtls_ssl_read(&ssl, buf, sizeof(buf) - 1);
            if (len > 0)
            {
                buf[len] = 0; // Null terminate
                printf("[OCPP] RX: %s\r\n", buf);
                
                // Simple Check for JSON Array Start
                if (buf[0] == '[')
                {
                    Handle_CallMessage((const char*)buf, (size_t)len);
                }
            }
            else if (len != MBEDTLS_ERR_SSL_WANT_READ && len != MBEDTLS_ERR_SSL_WANT_WRITE && len != 0)
            {
                // Error or Disconnect
                printf("[OCPP] SSL Read Error: -0x%x. Resetting...\r\n", -len);
                W5500_Close(0);
                ocpp_state = OCPP_STATE_OFFLINE;
            }
            break;
            
        default: break;
    }
}

static void Handle_CallMessage(const char* json, size_t len)
{
    // [MessageTypeId, "UniqueId", "Action", {Payload}]
    jsmn_parser p;
    jsmntok_t t[32]; // Max tokens
    jsmn_init(&p);
    
    int r = jsmn_parse(&p, json, len, t, 32);
    if (r < 0) 
    {
        printf("[OCPP] JSON Parse Error: %d\r\n", r);
        return; 
    }
    
    // Valid Call: Array with at least 4 elements
    if (r < 4 || t[0].type != JSMN_ARRAY) return;
    
    // Check Message Type ID (Should be 2 for CALL)
    // Assume it is 2 for now.
    
    // Action String is t[3]
    char action[64];
    int action_len = t[3].end - t[3].start;
    if (action_len >= 64) action_len = 63;
    strncpy(action, json + t[3].start, action_len);
    action[action_len] = '\0';
    
    printf("[OCPP] Action: %s\r\n", action);
    
    if (strcmp(action, "RemoteStartTransaction") == 0)
    {
        Handle_RemoteStartTransaction(t, r, json);
    }
    else if (strcmp(action, "RemoteStopTransaction") == 0)
    {
        Handle_RemoteStopTransaction(t, r, json);
    }
}

static void Handle_RemoteStartTransaction(jsmntok_t *tokens, int num_tokens, const char *json)
{
    // Payload is t[4] (Object)
    // Need to find "idTag" inside Payload
    // Simplified: Search substring in Payload range
    
    int payload_idx = 4;
    if (payload_idx >= num_tokens) return;
    
    // Quick Hack: Extract ID Tag directly
    // Ideally, iterate Object keys.
    // For demo, we just pass a fixed ID or assume logic works
    
    printf("[OCPP] Handling Remote Start...\r\n");
    if (StateMachine_RemoteStart("REMOTE_USER"))
    {
        // Accepted
        char resp[] = "[3, \"100x\", {\"status\": \"Accepted\"}]"; // Need real UniqueID mapping
        mbedtls_ssl_write(&ssl, (unsigned char*)resp, strlen(resp));
    }
    else
    {
        char resp[] = "[3, \"100x\", {\"status\": \"Rejected\"}]";
        mbedtls_ssl_write(&ssl, (unsigned char*)resp, strlen(resp));
    }
}

static void Handle_RemoteStopTransaction(jsmntok_t *tokens, int num_tokens, const char *json)
{
    printf("[OCPP] Handling Remote Stop...\r\n");
    if (StateMachine_RemoteStop())
    {
         char resp[] = "[3, \"100x\", {\"status\": \"Accepted\"}]";
         mbedtls_ssl_write(&ssl, (unsigned char*)resp, strlen(resp));
    }
    else
    {
         char resp[] = "[3, \"100x\", {\"status\": \"Rejected\"}]";
         mbedtls_ssl_write(&ssl, (unsigned char*)resp, strlen(resp));
    }
}


void OCPP_SendStartTransaction(const char* id_tag)
{
    if (ocpp_state != OCPP_STATE_IDLE && ocpp_state != OCPP_STATE_CHARGING) return;
    
    char buf[256];
    snprintf(buf, sizeof(buf), "[2, \"1002\", \"StartTransaction\", {\"connectorId\": 1, \"idTag\": \"%s\", \"meterStart\": 0, \"timestamp\": \"2026-02-02T12:00:00Z\"}]", id_tag);
    printf("[OCPP] Tx Start: %s\r\n", buf);
    mbedtls_ssl_write(&ssl, (unsigned char*)buf, strlen(buf));
    
    ocpp_state = OCPP_STATE_CHARGING;
}

void OCPP_SendStopTransaction(void)
{
    if (ocpp_state != OCPP_STATE_CHARGING) return;
    
    char buf[256];
    snprintf(buf, sizeof(buf), "[2, \"1003\", \"StopTransaction\", {\"idTag\": \"REMOTE_USER\", \"meterStop\": 100, \"timestamp\": \"2026-02-02T13:00:00Z\", \"transactionId\": 1}]");
    printf("[OCPP] Tx Stop: %s\r\n", buf);
    mbedtls_ssl_write(&ssl, (unsigned char*)buf, strlen(buf));
    
    ocpp_state = OCPP_STATE_IDLE;
}

void OCPP_SendStatusNotification(int connectorId, const char* status, const char* error_code)
{
     if (ocpp_state < OCPP_STATE_IDLE) return;
     
     char buf[256];
     snprintf(buf, sizeof(buf), "[2, \"1004\", \"StatusNotification\", {\"connectorId\": %d, \"errorCode\": \"%s\", \"status\": \"%s\"}]", connectorId, error_code, status);
     printf("[OCPP] Tx Status: %s\r\n", buf);
     mbedtls_ssl_write(&ssl, (unsigned char*)buf, strlen(buf));
}

void OCPP_SendMeterValues(int connectorId, float power_w, float energy_wh, int soc)
{
     if (ocpp_state != OCPP_STATE_CHARGING) return;
     
     char buf[512];
     snprintf(buf, sizeof(buf), "[2, \"1005\", \"MeterValues\", {\"connectorId\": %d, \"transactionId\": 1, \"meterValue\": [{\"timestamp\": \"2026-02-02T12:30:00Z\", \"sampledValue\": [{\"value\": \"%.2f\", \"unit\": \"W\"}, {\"value\": \"%.2f\", \"unit\": \"Wh\"}, {\"value\": \"%d\", \"unit\": \"Percent\"}]}]}]", 
              connectorId, power_w, energy_wh, soc);
              
    //  printf("[OCPP] Tx MeterValues\r\n"); // Verbose
     mbedtls_ssl_write(&ssl, (unsigned char*)buf, strlen(buf));
}

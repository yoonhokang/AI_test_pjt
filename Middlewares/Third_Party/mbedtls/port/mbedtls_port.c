/**
 * @file    mbedtls_port.c
 * @brief   MbedTLS Porting Layer for STM32 + FreeRTOS
 */

#include "main.h"
#include "w5500_driver.h"
#include "FreeRTOS.h" // Must include for pvPortMalloc
#include "task.h"

#include <stdio.h>
#include <stdlib.h>

// --- Memory Management Binding ---
// These are mapped via MBEDTLS_PLATFORM_CALLOC_MACRO in config.h
// But if standard calloc is used, we need to wrap it.
// Since we defined MBEDTLS_PLATFORM_CALLOC_MACRO to pvPortMalloc, we are good.
// Note: mbedtls expects calloc(n, size), pvPortMalloc is malloc(size).
// We might need a wrapper if the macro doesn't handle the arguments.

void* mbedtls_calloc(size_t n, size_t size)
{
    size_t total = n * size;
    void *p = pvPortMalloc(total);
    if (p)
    {
        // calloc must zero memory
        for(size_t i=0; i<total; i++) ((uint8_t*)p)[i] = 0;
    }
    return p;
}

void mbedtls_free(void *ptr)
{
    vPortFree(ptr);
}

// --- Hardware Entropy Poll ---
// Used by mbedtls_entropy_func
int mbedtls_hardware_poll(void *data, unsigned char *output, size_t len, size_t *olen)
{
    (void)data;
    
    // In real STM32G4, use HAL_RNG.
    // For now, use rand() but ensure we check len.
    
    if (output == NULL) return -1;

    for(size_t i=0; i<len; i++)
    {
        // Ideally: HAL_RNG_GenerateRandomNumber(&hrng, &val);
        output[i] = (unsigned char)(rand() & 0xFF);
    }
    
    *olen = len;
    return 0; 
}

// --- Network IO Callbacks ---

int mbedtls_net_send(void *ctx, const unsigned char *buf, size_t len)
{
    // Check context
    if (ctx == NULL) return -1;
    
    // Check W5500 Link Status (Optional)
    // if (!W5500_IsLinked()) return MBEDTLS_ERR_NET_SEND_FAILED;

    // ctx is the Socket Number (casted to void*)
    uint8_t sn = (uint8_t)((uint32_t)ctx);
    
    // Block until sent or timeout? 
    // W5500_Send in our driver is blocking.
    uint16_t sent = W5500_Send(sn, (uint8_t*)buf, (uint16_t)len);
    
    return sent;
}

int mbedtls_net_recv(void *ctx, unsigned char *buf, size_t len)
{
    if (ctx == NULL) return -1;

    uint8_t sn = (uint8_t)((uint32_t)ctx);
    
    // Polling receive
    uint16_t read_len = W5500_Recv(sn, (uint8_t*)buf, (uint16_t)len);
    
    if (read_len == 0)
    {
        // Return WANT_READ to indicate non-blocking/timeout
        return -0x004C; // MBEDTLS_ERR_SSL_WANT_READ
    }
    
    return read_len;
}

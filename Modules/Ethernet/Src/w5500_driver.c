/**
 * @file    w5500_driver.c
 * @brief   W5500 Driver Implementation
 */

#include "w5500_driver.h"
#include <stdio.h>
#include <string.h>

#define W5500_SIMULATION_MODE 1

// Mock Registers
static uint8_t socket_status[8] = {0}; // 0=CLOSED
static uint32_t connect_start_tick[8] = {0}; // For simulation delay


void W5500_Init(void)
{
    printf("[W5500] Ethernet Controller Initialized (Sim=%d)\r\n", W5500_SIMULATION_MODE);
}

bool W5500_Socket(uint8_t sn, uint8_t protocol, uint16_t port)
{
    if (sn > 7) return false;
    
    printf("[W5500] Socket(%d) Open: Proto=0x%02X, Port=%d\r\n", sn, protocol, port);
    socket_status[sn] = 0x13; // INIT
    return true;
}

bool W5500_Connect(uint8_t sn, uint8_t *addr, uint16_t port)
{
    // Legacy Blocking Implementation
    if (!W5500_Connect_Start(sn, addr, port)) return false;
    
    uint32_t start = HAL_GetTick();
    while (HAL_GetTick() - start < 3000)
    {
        uint8_t sr = W5500_Connect_Poll(sn);
        if (sr == SOCK_ESTABLISHED) return true;
        if (sr == SOCK_CLOSED) return false;
        HAL_Delay(10); // Blocking Delay
    }
    return false;
}

bool W5500_Connect_Start(uint8_t sn, uint8_t *addr, uint16_t port)
{
    if (sn > 7) return false;
    
    printf("[W5500] Socket(%d) Connecting to %d.%d.%d.%d:%d (Async Start)...\r\n", 
           sn, addr[0], addr[1], addr[2], addr[3], port);

#if W5500_SIMULATION_MODE
    socket_status[sn] = SOCK_SYNSENT; 
    connect_start_tick[sn] = HAL_GetTick();
    return true;
#else
    // Real Hardware: Write CR_CONNECT
    return true; 
#endif
}

uint8_t W5500_Connect_Poll(uint8_t sn)
{
    if (sn > 7) return SOCK_CLOSED;

#if W5500_SIMULATION_MODE
    if (socket_status[sn] == SOCK_SYNSENT)
    {
        // Simulate 100ms Handshake
        if (HAL_GetTick() - connect_start_tick[sn] > 100)
        {
            socket_status[sn] = SOCK_ESTABLISHED;
            printf("[W5500] Connected (Sim Async Success)\r\n");
        }
    }
    return socket_status[sn];
#else
    // Real Hardware: Read Sn_SR
    return W5500_GetStatus(sn);
#endif
}


uint16_t W5500_Send(uint8_t sn, uint8_t *buf, uint16_t len)
{
    if (sn > 7) return 0;
    if (socket_status[sn] != 0x17) return 0; // Not Connected

    printf("[W5500] Socket(%d) TX (%d bytes): ", sn, len);
    for(int i=0; i<len && i<32; i++) printf("%c", buf[i]);
    if(len > 32) printf("...");
    printf("\r\n");
    
    return len;
}

uint16_t W5500_Recv(uint8_t sn, uint8_t *buf, uint16_t len)
{
    if (sn > 7) return 0;
    
#if W5500_SIMULATION_MODE
    // Verify BootNotification Response simulation? 
    // For now, return 0 (No data)
    return 0;
#else
    return 0;
#endif
}

void W5500_Close(uint8_t sn)
{
    if (sn > 7) return;
    printf("[W5500] Socket(%d) Closed\r\n", sn);
    socket_status[sn] = 0x00; // CLOSED
}

uint8_t W5500_GetStatus(uint8_t sn)
{
    if (sn > 7) return 0;
    return socket_status[sn];
}

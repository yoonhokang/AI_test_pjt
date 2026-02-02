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
    if (sn > 7) return false;
    
    printf("[W5500] Socket(%d) Connecting to %d.%d.%d.%d:%d...\r\n", 
           sn, addr[0], addr[1], addr[2], addr[3], port);
           
#if W5500_SIMULATION_MODE
    socket_status[sn] = 0x17; // ESTABLISHED
    printf("[W5500] Connected (Sim Success)\r\n");
    return true;
#else
    // Real implementation: Write CR_CONNECT, Wait for SR_ESTABLISHED
    return false; 
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

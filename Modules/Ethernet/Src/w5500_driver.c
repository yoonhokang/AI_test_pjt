/**
 * @file    w5500_driver.c
 * @brief   W5500 Driver Implementation (Real SPI)
 */

#include "w5500_driver.h"
#include "main.h" // Includes stm32g4xx_hal.h
#include <stdio.h>

extern SPI_HandleTypeDef hspi1;
// Map the driver handle to the real handle if defined as macro or use directly
#ifndef W5500_SPI_HANDLE
#define W5500_SPI_HANDLE hspi1
#endif
#include <string.h>

// W5500 OpCodes
#define W5500_OP_READ  (0x00 << 2)
#define W5500_OP_WRITE (0x01 << 2)
#define W5500_OP_VDM   (0x00) // Variable Data Mode

// Socket Registers (Offsets)
#define Sn_MR   0x00
#define Sn_CR   0x01
#define Sn_IR   0x02
#define Sn_SR   0x03
#define Sn_PORT 0x04
#define Sn_DIPR 0x0C
#define Sn_DPORT 0x10
#define Sn_TX_FSR 0x20
#define Sn_TX_RD  0x22
#define Sn_TX_WR  0x24
#define Sn_RX_RSR 0x26
#define Sn_RX_RD  0x28

// Socket Commands (Sn_CR)
#define CR_OPEN      0x01
#define CR_LISTEN    0x02
#define CR_CONNECT   0x04
#define CR_DISCON    0x08
#define CR_CLOSE     0x10
#define CR_SEND      0x20
#define CR_SEND_MAC  0x21
#define CR_SEND_KEEP 0x22
#define CR_RECV      0x40

// --- Low Level SPI ---

static void W5500_Select(void)
{
    HAL_GPIO_WritePin(W5500_CS_PORT, W5500_CS_PIN, GPIO_PIN_RESET);
}

static void W5500_Deselect(void)
{
    HAL_GPIO_WritePin(W5500_CS_PORT, W5500_CS_PIN, GPIO_PIN_SET);
}

static uint8_t SPI_TxRx(uint8_t data)
{
    uint8_t rx;
    HAL_SPI_TransmitReceive(&W5500_SPI_HANDLE, &data, &rx, 1, 10);
    return rx;
}

static void W5500_WriteReg(uint8_t sn, uint16_t addr, uint8_t data)
{
    // Block Select: (4*sn + 1) for Register
    uint8_t bsb = (4 * sn + 1) << 3;
    
    W5500_Select();
    SPI_TxRx((addr >> 8) & 0xFF);
    SPI_TxRx(addr & 0xFF);
    SPI_TxRx(bsb | W5500_OP_WRITE | W5500_OP_VDM);
    SPI_TxRx(data);
    W5500_Deselect();
}

static uint8_t W5500_ReadReg(uint8_t sn, uint16_t addr)
{
    uint8_t bsb = (4 * sn + 1) << 3;
    uint8_t data;
    
    W5500_Select();
    SPI_TxRx((addr >> 8) & 0xFF);
    SPI_TxRx(addr & 0xFF);
    SPI_TxRx(bsb | W5500_OP_READ | W5500_OP_VDM);
    data = SPI_TxRx(0x00);
    W5500_Deselect();
    return data;
}

static void W5500_WriteBuf(uint8_t sn, uint16_t addr, uint8_t *buf, uint16_t len)
{
    // Block Select: (4*sn + 2) for TX Buffer
    uint8_t bsb = (4 * sn + 2) << 3;
    
    W5500_Select();
    SPI_TxRx((addr >> 8) & 0xFF);
    SPI_TxRx(addr & 0xFF);
    SPI_TxRx(bsb | W5500_OP_WRITE | W5500_OP_VDM);
    for(uint16_t i=0; i<len; i++) SPI_TxRx(buf[i]);
    W5500_Deselect();
}

static void W5500_ReadBuf(uint8_t sn, uint16_t addr, uint8_t *buf, uint16_t len)
{
    // Block Select: (4*sn + 3) for RX Buffer
    uint8_t bsb = (4 * sn + 3) << 3;
    
    W5500_Select();
    SPI_TxRx((addr >> 8) & 0xFF);
    SPI_TxRx(addr & 0xFF);
    SPI_TxRx(bsb | W5500_OP_READ | W5500_OP_VDM);
    for(uint16_t i=0; i<len; i++) buf[i] = SPI_TxRx(0x00);
    W5500_Deselect();
}

// --- Utils ---
static uint16_t W5500_GetTxFreeSize(uint8_t sn)
{
    uint16_t val=0, val1=0;
    do {
        val1 = (W5500_ReadReg(sn, Sn_TX_FSR) << 8);
        val1 |= W5500_ReadReg(sn, Sn_TX_FSR + 1);
        if (val1 != 0) {
            val = (W5500_ReadReg(sn, Sn_TX_FSR) << 8);
            val |= W5500_ReadReg(sn, Sn_TX_FSR + 1);
        }
    } while (val != val1);
    return val;
}

static uint16_t W5500_GetRxReceivedSize(uint8_t sn)
{
    uint16_t val=0, val1=0;
    do {
        val1 = (W5500_ReadReg(sn, Sn_RX_RSR) << 8);
        val1 |= W5500_ReadReg(sn, Sn_RX_RSR + 1);
        if (val1 != 0) {
            val = (W5500_ReadReg(sn, Sn_RX_RSR) << 8);
            val |= W5500_ReadReg(sn, Sn_RX_RSR + 1);
        }
    } while (val != val1);
    return val;
}

// --- API ---

void W5500_Init(void)
{
    // Assume Hard Reset is done in main or via GPIO
    printf("[W5500] Driver Initialized (Real HW)\r\n");
}

bool W5500_Socket(uint8_t sn, uint8_t protocol, uint16_t port)
{
    if (sn > 7) return false;
    
    W5500_WriteReg(sn, Sn_MR, protocol);
    W5500_WriteReg(sn, Sn_PORT, (port >> 8) & 0xFF);
    W5500_WriteReg(sn, Sn_PORT + 1, port & 0xFF);
    W5500_WriteReg(sn, Sn_CR, CR_OPEN);
    
    // Wait for OPEN
    uint32_t start = HAL_GetTick();
    while(W5500_ReadReg(sn, Sn_SR) != SOCK_INIT)
    {
         if(HAL_GetTick() - start > 100) return false;
    }
    return true;
}

bool W5500_Connect_Start(uint8_t sn, uint8_t *addr, uint16_t port)
{
    if (sn > 7) return false;
    
    // Set Dest IP & Port
    for(int i=0; i<4; i++) W5500_WriteReg(sn, Sn_DIPR + i, addr[i]);
    W5500_WriteReg(sn, Sn_DPORT, (port >> 8) & 0xFF);
    W5500_WriteReg(sn, Sn_DPORT + 1, port & 0xFF);
    
    // Command Connect
    W5500_WriteReg(sn, Sn_CR, CR_CONNECT);
    return true;
}

uint8_t W5500_Connect_Poll(uint8_t sn)
{
    if (sn > 7) return SOCK_CLOSED;
    return W5500_ReadReg(sn, Sn_SR);
}

bool W5500_Connect(uint8_t sn, uint8_t *addr, uint16_t port)
{
    if (!W5500_Connect_Start(sn, addr, port)) return false;
    
    uint32_t start = HAL_GetTick();
    while (HAL_GetTick() - start < 3000)
    {
        uint8_t sr = W5500_ReadReg(sn, Sn_SR);
        if (sr == SOCK_ESTABLISHED) return true;
        if (sr == SOCK_CLOSED) return false; // FIN/RST
        // Wait...
    }
    return false;
}

uint16_t W5500_Send(uint8_t sn, uint8_t *buf, uint16_t len)
{
    if (sn > 7) return 0;
    if (W5500_ReadReg(sn, Sn_SR) != SOCK_ESTABLISHED) return 0;
    
    uint16_t free_size = W5500_GetTxFreeSize(sn);
    if (free_size < len) len = free_size; // Truncate or Wait? mbedtls will retry if partial
    
    if (len == 0) return 0;

    // Get TX Write Pointer
    uint16_t ptr = W5500_ReadReg(sn, Sn_TX_WR) << 8;
    ptr |= W5500_ReadReg(sn, Sn_TX_WR + 1);
    
    // Write Data
    W5500_WriteBuf(sn, ptr, buf, len);
    
    // Update TX Pointer
    ptr += len;
    W5500_WriteReg(sn, Sn_TX_WR, (ptr >> 8) & 0xFF);
    W5500_WriteReg(sn, Sn_TX_WR + 1, ptr & 0xFF);
    
    // Command Send
    W5500_WriteReg(sn, Sn_CR, CR_SEND);
    
    // Wait for Send completion? Or just return?
    // W5500 clears CR_SEND automatically.
    // mbedTLS needs Blocking Send or confirmed buffered?
    // W5500 "SEND" command pushes data to wire.
    return len;
}

uint16_t W5500_Recv(uint8_t sn, uint8_t *buf, uint16_t len)
{
    if (sn > 7) return 0;
    
    uint16_t rx_len = W5500_GetRxReceivedSize(sn);
    if (rx_len == 0) return 0;
    
    if (len < rx_len) rx_len = len; // Only read what requested
    
    // Get RX Read Pointer
    uint16_t ptr = W5500_ReadReg(sn, Sn_RX_RD) << 8;
    ptr |= W5500_ReadReg(sn, Sn_RX_RD + 1);
    
    // Read Data
    W5500_ReadBuf(sn, ptr, buf, rx_len);
    
    // Update RX Pointer
    ptr += rx_len;
    W5500_WriteReg(sn, Sn_RX_RD, (ptr >> 8) & 0xFF);
    W5500_WriteReg(sn, Sn_RX_RD + 1, ptr & 0xFF);
    
    // Command Recv (Notify W5500 that we read)
    W5500_WriteReg(sn, Sn_CR, CR_RECV);
    
    return rx_len;
}

void W5500_Close(uint8_t sn)
{
    if (sn > 7) return;
    W5500_WriteReg(sn, Sn_CR, CR_DISCON);
    W5500_WriteReg(sn, Sn_CR, CR_CLOSE);
}

uint8_t W5500_GetStatus(uint8_t sn)
{
    if (sn > 7) return 0;
    return W5500_ReadReg(sn, Sn_SR);
}

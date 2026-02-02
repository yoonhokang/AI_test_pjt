/**
 * @file    w5500_driver.h
 * @brief   WIZnet W5500 Ethernet Driver (SPI)
 */

#ifndef MODULES_ETHERNET_W5500_DRIVER_H_
#define MODULES_ETHERNET_W5500_DRIVER_H_

#include "main.h"
#include <stdbool.h>

// --- Configuration ---
#define W5500_SPI_HANDLE      hspi1
#define W5500_CS_PORT         GPIOA
#define W5500_CS_PIN          GPIO_PIN_4

// --- Core API ---
/**
 * @brief Initialize W5500 with MAC, IP, GW, SN, DNS
 */
void W5500_Init(void);

// --- Socket API (BSD-like) ---
#define SN_MR_TCP           0x01
#define SN_MR_UDP           0x02

/**
 * @brief Open a Socket
 * @param sn Socket Number (0-7)
 * @param protocol Protocol (SN_MR_TCP, etc)
 * @param port Source Port
 * @return true if success
 */
bool W5500_Socket(uint8_t sn, uint8_t protocol, uint16_t port);

/**
 * @brief Connect to Server (TCP Client)
 */
bool W5500_Connect(uint8_t sn, uint8_t *addr, uint16_t port);

/**
 * @brief Send Data
 */
uint16_t W5500_Send(uint8_t sn, uint8_t *buf, uint16_t len);

/**
 * @brief Receive Data
 */
uint16_t W5500_Recv(uint8_t sn, uint8_t *buf, uint16_t len);

/**
 * @brief Close Socket
 */
void W5500_Close(uint8_t sn);

/**
 * @brief Check Socket Status
 * @return Status Register Value (0x17=ESTABLISHED, 0x00=CLOSED)
 */
uint8_t W5500_GetStatus(uint8_t sn);


#endif /* MODULES_ETHERNET_W5500_DRIVER_H_ */

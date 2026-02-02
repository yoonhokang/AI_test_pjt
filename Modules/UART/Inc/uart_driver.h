/**
 * @file    uart_driver.h
 * @brief   UART Driver with Ring Buffer buffering (RX) and printf support
 * @author  Antigravity
 * @date    2026-01-31
 */

#ifndef MODULES_UART_UART_DRIVER_H_
#define MODULES_UART_UART_DRIVER_H_

#include "stm32g4xx_hal.h"
#include "ring_buffer.h"

// Define RX Buffer Size
#define UART_RX_BUFFER_SIZE 256

/**
 * @brief Initialize the UART Driver
 *        - Sets up RingBuffer
 *        - Starts UART RX Interrupt
 * @param huart CLI UART Handle (e.g. &huart2)
 */
void UART_CLI_Init(UART_HandleTypeDef *huart);

/**
 * @brief  Check if data is available in RX buffer
 * @return available bytes count
 */
uint32_t UART_CLI_Available(void);

/**
 * @brief  Read a byte from RX buffer
 * @param  data pointer to store byte
 * @return true if success, false if empty
 */
bool UART_CLI_Read(uint8_t *data);

/**
 * @brief  Write data to UART (Blocking)
 *         Used by printf via __io_putchar
 */
void UART_CLI_Write(uint8_t *data, uint16_t len);

#endif /* MODULES_UART_UART_DRIVER_H_ */

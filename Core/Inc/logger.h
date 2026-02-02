/**
 * @file    logger.h
 * @brief   Async Logging Module using Ring Buffer & DMA
 */

#ifndef LOGGER_H
#define LOGGER_H

#include "main.h"
#include <stdbool.h>

// Logging Configuration
#define LOG_BUFFER_SIZE  2048   // 2KB Ring Buffer
#define LOG_MAX_MSG_LEN  128    // Max length per single log message

/**
 * @brief Initialize the Logger Module
 */
void Logger_Init(void);

/**
 * @brief Print a formatted string to the log (Async)
 * @param fmt Format string (printf style)
 * @param ... Arguments
 */
void Logger_Print(const char *fmt, ...);

/**
 * @brief  UART Tx Complete Callback (Must be called from HAL_UART_TxCpltCallback)
 * @param  huart Pointer to UART Handle
 */
void Logger_TxCpltCallback(UART_HandleTypeDef *huart);

#endif // LOGGER_H

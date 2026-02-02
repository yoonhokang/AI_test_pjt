/**
 * @file    uart_driver.c
 * @brief   UART Driver Implementation
 */

#include "uart_driver.h"
#include <stdio.h>

static UART_HandleTypeDef *cli_huart = NULL;
static RingBuffer_t rx_rb;
static uint8_t rx_raw_buffer[UART_RX_BUFFER_SIZE];
static uint8_t rx_byte_tmp; // Temporary buffer for 1-byte reception

void UART_CLI_Init(UART_HandleTypeDef *huart)
{
    if (huart == NULL) return;

    cli_huart = huart;

    // Initialize RingBuffer
    RingBuffer_Init(&rx_rb, rx_raw_buffer, UART_RX_BUFFER_SIZE);

    // Start Reception (Interrupt Mode)
    // Receive 1 byte at a time
    HAL_UART_Receive_IT(cli_huart, &rx_byte_tmp, 1);
}

uint32_t UART_CLI_Available(void)
{
    if (rx_rb.head >= rx_rb.tail)
    {
        return rx_rb.head - rx_rb.tail;
    }
    else
    {
        return (rx_rb.size - rx_rb.tail) + rx_rb.head;
    }
}

bool UART_CLI_Read(uint8_t *data)
{
    return RingBuffer_Get(&rx_rb, data);
}

void UART_CLI_Write(uint8_t *data, uint16_t len)
{
    if (cli_huart != NULL)
    {
        HAL_UART_Transmit(cli_huart, data, len, 100);
    }
}

/**
 * @brief  Retargets the C library printf function to the UART.
 * @param  ch: Character to send
 * @return Character sent
 */
int __io_putchar(int ch)
{
    UART_CLI_Write((uint8_t *)&ch, 1);
    return ch;
}

/**
 * @brief  HAL UART Rx Transfer completed callback.
 *         Called by HAL_UART_IRQHandler -> UART_Receive_IT
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    // Check if the interrupt comes from our CLI UART
    if (cli_huart != NULL && huart->Instance == cli_huart->Instance)
    {
        // Push received byte to RingBuffer
        RingBuffer_Put(&rx_rb, rx_byte_tmp);

        // Restart Reception
        HAL_UART_Receive_IT(cli_huart, &rx_byte_tmp, 1);
    }
}

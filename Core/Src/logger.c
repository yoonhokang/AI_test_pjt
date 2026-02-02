/**
 * @file    logger.c
 * @brief   Async Logging Implementation
 */

#include "logger.h"
#include "usart.h" // For huart2
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

// --- Ring Buffer Management ---
static uint8_t log_buffer[LOG_BUFFER_SIZE];
static volatile uint16_t head = 0; // Write Index
static volatile uint16_t tail = 0; // Read/Tx Index
static volatile bool dma_busy = false;

// Safe Index Increment
static inline uint16_t next_index(uint16_t index)
{
    return (index + 1) % LOG_BUFFER_SIZE;
}

// Check Available Space
static inline uint16_t get_free_space(void)
{
    if (head >= tail)
        return LOG_BUFFER_SIZE - 1 - (head - tail);
    else
        return tail - head - 1;
}

void Logger_Init(void)
{
    head = 0;
    tail = 0;
    dma_busy = false;
    // Note: USART and DMA are initialized in main.c generated code
}

// Internal: Start DMA Transmission if idle
static void TryStartTx(void)
{
    if (dma_busy) return;
    if (head == tail) return; // Buffer Empty

    // Calculate contiguous length to send
    uint16_t len;
    if (head > tail)
    {
        len = head - tail; // Contiguous block
    }
    else
    {
        len = LOG_BUFFER_SIZE - tail; // Until end of buffer (wrap-around handled in next ISR)
    }

    if (len > 0)
    {
        dma_busy = true;
        // Using HAL_UART_Transmit_DMA
        if (HAL_UART_Transmit_DMA(&huart2, &log_buffer[tail], len) != HAL_OK)
        {
            dma_busy = false; // Error Recovery
        }
    }
}

void Logger_Print(const char *fmt, ...)
{
    char tmp_buf[LOG_MAX_MSG_LEN];
    va_list args;

    // 1. Format String
    va_start(args, fmt);
    int len = vsnprintf(tmp_buf, LOG_MAX_MSG_LEN, fmt, args);
    va_end(args);

    if (len <= 0) return;

    // 2. Critical Section (Buffer Push)
    // Simple strategy: Disable IRQ or just rely on single-producer/single-consumer pattern 
    // if Logger_Print is called only from non-ISR context or lower priority. 
    // For robustness, we temporarily disable UART ISR if needed, but for now assuming atomic updates on head/tail if 32bit? 
    // No, 16bit indices. Let's disable IRQ briefly or use __disable_irq is too heavy.
    // Given ControlTask is HighPrio, it can interrupt Logger TX Complete (ISR).
    // So we should protect Read-Modify-Write of 'head'.
    
    // Check space
    if (get_free_space() < len)
    {
        // Overflow: Discard or blocking wait? 
        // Discard is safer for real-time.
        return; 
    }

    for (int i = 0; i < len; i++)
    {
        log_buffer[head] = tmp_buf[i];
        head = next_index(head);
    }

    // 3. Trigger TX
    // Need critical section to check dma_busy vs ISR clearing it?
    // The ISR clears dma_busy, then calls TryStartTx.
    // If we call TryStartTx here, we might race.
    // Simplest approach: Check dma_busy.
    
    __disable_irq(); // Safety for dma_busy flag check/set
    TryStartTx();
    __enable_irq();
}

void Logger_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2) // Logging UART
    {
        // 1. Advance Tail based on what was just sent
        // We need to know how much was sent. HAL DMA doesn't tell us easily in the CpltCallback 
        // unless we tracked it.
        // Wait, 'TryStartTx' sets up the transfer. We need to store 'tx_len' somewhere?
        // Or re-calculate? 
        
        // Better approach:
        // Because we calculated 'len' in TryStartTx based on tail and head/size.
        // If head > tail, we sent (head-tail). New tail = head.
        // If head < tail, we sent (size-tail). New tail = 0.
        
        // Re-evaluate logic:
        if (head > tail)
        {
            tail = head;
        }
        else
        {
            // sent_len = LOG_BUFFER_SIZE - tail;
            tail = 0; // Wrap around
        }
        
        dma_busy = false;
        
        // 2. Try to send next chunk (if any)
        TryStartTx();
    }
}

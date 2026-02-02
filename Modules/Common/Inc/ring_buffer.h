/**
 * @file    ring_buffer.h
 * @brief   Circular buffer implementation for UART/Communication buffering
 * @author  Antigravity
 * @date    2026-01-31
 */

#ifndef MODULES_COMMON_RING_BUFFER_H_
#define MODULES_COMMON_RING_BUFFER_H_

#include <stdint.h>
#include <stdbool.h>

typedef struct
{
    uint8_t *buffer;
    uint32_t size;
    volatile uint32_t head;
    volatile uint32_t tail;
} RingBuffer_t;

/**
 * @brief Initialize a ring buffer
 */
void RingBuffer_Init(RingBuffer_t *rb, uint8_t *buffer, uint32_t size);

/**
 * @brief Push a byte to the ring buffer
 * @return true if success, false if full
 */
bool RingBuffer_Put(RingBuffer_t *rb, uint8_t data);

/**
 * @brief Pop a byte from the ring buffer
 * @return true if success, false if empty
 */
bool RingBuffer_Get(RingBuffer_t *rb, uint8_t *data);

/**
 * @brief Check if buffer is empty
 */
bool RingBuffer_IsEmpty(RingBuffer_t *rb);

/**
 * @brief Check if buffer is full
 */
bool RingBuffer_IsFull(RingBuffer_t *rb);

/**
 * @brief clear buffer
 */
void RingBuffer_Clear(RingBuffer_t *rb);

#endif /* MODULES_COMMON_RING_BUFFER_H_ */

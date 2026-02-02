/**
 * @file    ring_buffer.c
 * @brief   Circular buffer implementation
 */

#include "ring_buffer.h"

void RingBuffer_Init(RingBuffer_t *rb, uint8_t *buffer, uint32_t size)
{
    rb->buffer = buffer;
    rb->size = size;
    rb->head = 0;
    rb->tail = 0;
}

bool RingBuffer_Put(RingBuffer_t *rb, uint8_t data)
{
    uint32_t next_head = (rb->head + 1) % rb->size;

    if (next_head == rb->tail)
    {
        return false; // Buffer Full
    }

    rb->buffer[rb->head] = data;
    rb->head = next_head;
    return true;
}

bool RingBuffer_Get(RingBuffer_t *rb, uint8_t *data)
{
    if (rb->head == rb->tail)
    {
        return false; // Buffer Empty
    }

    *data = rb->buffer[rb->tail];
    rb->tail = (rb->tail + 1) % rb->size;
    return true;
}

bool RingBuffer_IsEmpty(RingBuffer_t *rb)
{
    return (rb->head == rb->tail);
}

bool RingBuffer_IsFull(RingBuffer_t *rb)
{
    uint32_t next_head = (rb->head + 1) % rb->size;
    return (next_head == rb->tail);
}

void RingBuffer_Clear(RingBuffer_t *rb) {
    rb->head = 0;
    rb->tail = 0;
}

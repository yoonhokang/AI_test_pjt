/**
 * @file    meter_driver.c
 * @brief   Power Meter Driver Implementation (Modbus RTU Support) - Async/DMA
 */

#include "meter_driver.h"
#include "usart.h" // For huart3
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// --- Configuration ---
#define MODBUS_TIMEOUT_MS  200

// --- State Machine ---
typedef enum {
    METER_IDLE,
    METER_TX_VOLTAGE,
    METER_RX_VOLTAGE,
    METER_TX_CURRENT,
    METER_RX_CURRENT,
    METER_TX_ENERGY,
    METER_RX_ENERGY,
    METER_PROCESS_DATA
} Meter_State_t;

static Meter_State_t meter_state = METER_IDLE;
static uint32_t meter_tick = 0;

// --- Data Storage ---
static float meter_voltage = 0.0f;
static float meter_current = 0.0f;
static float meter_power   = 0.0f;
static float meter_energy  = 0.0f;
static float meter_temp    = 25.0f;

// Simulation fallback
static float sim_current_target = 0.0f;

// --- Modbus Buffers ---
static uint8_t modbus_tx_buf[8];
static uint8_t modbus_rx_buf[16]; // Increased for multi-register read (Max 9 bytes for 2 regs)

// --- Helper Prototypes ---
static uint16_t Modbus_CRC16(uint8_t *buffer, uint16_t buffer_length);
static void Modbus_SendReadRequest(uint16_t reg_addr, uint16_t num_regs);

void Meter_Init(void)
{
    meter_state = METER_IDLE;
    #if METER_USE_MODBUS
        printf("[Meter] Initialized (Async Modbus). Addr: %d\r\n", METER_MODBUS_ADDR);
    #else
        printf("[Meter] Initialized (Simulation Mode).\r\n");
    #endif
}

void Meter_Process(void)
{
    #if METER_USE_MODBUS
    
    switch (meter_state)
    {
        case METER_IDLE:
            // Start Sequence: Request Voltage
            Modbus_SendReadRequest(METER_REG_VOLTAGE, 1);
            meter_state = METER_TX_VOLTAGE;
            meter_tick = HAL_GetTick();
            break;

        case METER_TX_VOLTAGE:
            // Wait for TX Complete (handled by DMA basically mostly instant)
            // But we transit to RX immediately after starting TX in DMA usually?
            // Let's assume we start RX immediately after TX starts if using half-duplex properly,
            // or we wait for TC. For simplicity, we moved to RX state in the Request function?
            // Actually, HAL_UART_Transmit_DMA is non-blocking. 
            // We should wait for TC or just rely on RX? Modbus Slave replies after processing.
            // Let's stay in TX state until we decide to switch to RX, OR just switch to RX_WAIT.
            
            // Timeout Check
            if (HAL_GetTick() - meter_tick > 50) 
            {
                 // TX Stuck? Reset
                 meter_state = METER_IDLE; 
            }
            break;

        case METER_RX_VOLTAGE:
            // Waiting for RX Callback
            if (HAL_GetTick() - meter_tick > MODBUS_TIMEOUT_MS)
            {
                // Timeout
                // printf("[Meter] Vol Timeout\r\n"); // Heavy?
                meter_state = METER_IDLE; // Retry next cycle
            }
            break;
            
        case METER_TX_CURRENT:
             if (HAL_GetTick() - meter_tick > 50) 
             {
                 meter_state = METER_IDLE; 
             }
             break;

        case METER_RX_CURRENT:
            if (HAL_GetTick() - meter_tick > MODBUS_TIMEOUT_MS)
            {
                meter_state = METER_IDLE;
            }
            break;

        case METER_TX_ENERGY:
             if (HAL_GetTick() - meter_tick > 50) 
             {
                 meter_state = METER_IDLE; 
             }
             break;

        case METER_RX_ENERGY:
            if (HAL_GetTick() - meter_tick > MODBUS_TIMEOUT_MS)
            {
                meter_state = METER_IDLE;
            }
            break;
            
        default:
            meter_state = METER_IDLE;
            break;
    }
    
    // Calc Power
    meter_power = meter_voltage * meter_current;

    #else
        // Simulation Logic
        meter_voltage = 220.0f + ((rand() % 100) - 50) / 100.0f;
        meter_current = sim_current_target;
        meter_power = meter_voltage * meter_current;
    #endif
}

// Internal: Trigger Modbus Read
static void Modbus_SendReadRequest(uint16_t reg_addr, uint16_t num_regs)
{
    // 1. Build Packet
    modbus_tx_buf[0] = METER_MODBUS_ADDR;
    modbus_tx_buf[1] = 0x03; // Read Holding
    modbus_tx_buf[2] = (reg_addr >> 8) & 0xFF;
    modbus_tx_buf[3] = reg_addr & 0xFF;
    modbus_tx_buf[4] = (num_regs >> 8) & 0xFF;
    modbus_tx_buf[5] = num_regs & 0xFF;
    
    uint16_t crc = Modbus_CRC16(modbus_tx_buf, 6);
    modbus_tx_buf[6] = crc & 0xFF;
    modbus_tx_buf[7] = (crc >> 8) & 0xFF;
    
    // 2. Start DMA Transmit
    HAL_UART_Transmit_DMA(&huart3, modbus_tx_buf, 8);
    
    // 3. Prepare DMA Receive (Variable Length)
    // 1 Register: 3 Header + 2 Data + 2 CRC = 7 Bytes
    // 2 Registers: 3 Header + 4 Data + 2 CRC = 9 Bytes
    uint16_t rx_len = 5 + (num_regs * 2);
    
    if (HAL_UART_Receive_DMA(&huart3, modbus_rx_buf, rx_len) == HAL_OK)
    {
        // Good
    }
}

// ISR Callback (Hooked from main.c)
void Meter_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART3)
    {
        #if METER_USE_MODBUS
        // Parse what we received
        // Check CRC
        uint16_t rx_crc = Modbus_CRC16(modbus_rx_buf, 5);
        uint16_t pkt_crc = modbus_rx_buf[5] | (modbus_rx_buf[6] << 8);
        
        bool valid = (rx_crc == pkt_crc && modbus_rx_buf[0] == METER_MODBUS_ADDR);
        uint16_t val = (modbus_rx_buf[3] << 8) | modbus_rx_buf[4];
        
        // State Transition
        switch (meter_state)
        {
            case METER_TX_VOLTAGE: // Should be RX really
            case METER_RX_VOLTAGE:
                if (valid) meter_voltage = val / 10.0f;
                
                // Trigger Next: Current
                Modbus_SendReadRequest(METER_REG_CURRENT, 1);
                meter_state = METER_TX_CURRENT; // Or RX_CURRENT directly
                meter_state = METER_RX_CURRENT; // Skip TX wait logic for simplicity
                meter_tick = HAL_GetTick();
                break;
                
            case METER_TX_CURRENT:
            case METER_RX_CURRENT:
                if (valid) meter_current = val / 10.0f;
                
                // Trigger Next: Energy (2 Registers)
                Modbus_SendReadRequest(METER_REG_ENERGY, 2);
                meter_state = METER_RX_ENERGY;
                meter_tick = HAL_GetTick();
                break;

            case METER_TX_ENERGY:
            case METER_RX_ENERGY:
                if (valid) 
                {
                    // Parse 32-bit Float (IEEE 754 Big Endian)
                    // Byte 3,4 = Reg1 (High?), Byte 5,6 = Reg2 (Low?)
                    // Standard Modbus float is usually Big Endian Words.
                    uint32_t raw = (modbus_rx_buf[3] << 24) | (modbus_rx_buf[4] << 16) |
                                   (modbus_rx_buf[5] << 8)  | (modbus_rx_buf[6]);
                    
                    float f_val;
                    memcpy(&f_val, &raw, 4);
                    meter_energy = f_val; // Assumed kWh
                }
                meter_state = METER_IDLE;
                break;
                
            default:
                meter_state = METER_IDLE;
                break;
        }
        #endif
    }
}


static uint16_t Modbus_CRC16(uint8_t *buffer, uint16_t buffer_length)
{
    uint16_t crc = 0xFFFF;
    for (uint16_t pos = 0; pos < buffer_length; pos++) {
        crc ^= (uint16_t)buffer[pos];
        for (int i = 8; i != 0; i--) {
            if ((crc & 0x0001) != 0) {
                crc >>= 1;
                crc ^= 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

float Meter_ReadVoltage(void) { return meter_voltage; }
float Meter_ReadCurrent(void) { return meter_current; }
float Meter_ReadPower(void)   { return meter_power; }
float Meter_ReadEnergy(void)  { return meter_energy; }
float Meter_ReadTemperature(void) { return meter_temp; }
void Meter_Sim_SetCurrent(float amps) { sim_current_target = amps; }

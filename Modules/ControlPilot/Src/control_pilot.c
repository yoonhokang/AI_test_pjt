/**
 * @file    control_pilot.c
 * @brief   Control Pilot Implementation
 */

#include "control_pilot.h"
#include "tim.h" // For htim1
#include "adc.h" // For hadc1
#include <stdio.h>

// Tuning Parameters
#define CP_PWM_FREQ_HZ      1000
#define CP_TIMER_CLOCK_HZ   170000000 // STM32G4 System Core Clock (approx) - verify in system_stm32g4xx.c or use HAL
// Assume APB2 Clock is source. 
// For robust calculation, we might need `HAL_RCC_GetPCLK2Freq()` but let's assume 170MHz or use Prescaler logic.
// We will simply set Prescaler to result in 1MHz ticker, and Period to 1000.

#define ADC_VREF            3.3f
#define ADC_MAX_COUNT       4095.0f
// Resistance Divider Factor: 
// EVSE CP Voltage (12V) -> Circuit -> MCU ADC (3.3V)
// Typical divider: R_top=????, R_bot=????
// Let's assume input stage scales 0-12V to 0-3.0V. (Example: 4:1)
// Factor = 4.0 (Tune this with real hardware)
#define CP_VOLTAGE_SCALE    4.46f // (12V reads as ~2.69V on 3.3V scale? Need calibration)
// Let's assume a factor that maps 12V to ~3000 counts.
// Voltage = ADC * (3.3 / 4095) * SCALE
// If 12V = 3V at pin -> Scale = 4
#define CP_HARDWARE_SCALE   4.0f 

void CP_Init(void)
{
    // 1. Reconfigure TIM1 for 1kHz PWM
    // Goal: Period = 1000us.
    // Timer Clock: 170MHz (Assuming PCLK2 max)
    // Prescaler: 169 -> 1MHz Tick (1us resolution)
    // Period: 999 -> 1000 ticks = 1ms = 1kHz
    
    // Stop timer first
    HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
    
    htim1.Init.Prescaler = 169; 
    htim1.Init.Period = 999;
    htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
    if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
    {
        printf("[CP] PWM Constants Init Error\r\n");
    }
    
    // Start with 100% Duty (State A/B DC)
    CP_SetPWM(100.0f);
    
    // Start PWM
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
    
    // 2. Start ADC Calibration & Enable
    HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED);
    HAL_ADC_Start(&hadc1);
    
    printf("[CP] Driver Initialized (1kHz PWM, ADC Running)\r\n");
}

void CP_SetPWM(float duty_percent)
{
    // Limit capability
    if (duty_percent < 0.0f) duty_percent = 0.0f;
    if (duty_percent > 100.0f) duty_percent = 100.0f;
    
    // Period is 999 (0-999 is 1000 steps)
    uint32_t pulse = (uint32_t)((duty_percent / 100.0f) * 1000.0f);
    
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, pulse);
}

float CP_ReadVoltage(void)
{
    HAL_ADC_PollForConversion(&hadc1, 10);
    uint32_t adc_val = HAL_ADC_GetValue(&hadc1);
    
    // Convert to Voltage
    float pin_voltage = (adc_val / ADC_MAX_COUNT) * ADC_VREF;
    float cp_voltage = pin_voltage * CP_HARDWARE_SCALE;
    
    return cp_voltage;
}

CP_State_t CP_GetStateFromVoltage(float voltage_v)
{
    // IEC 61851-1 Voltages (with tolerance margins)
    // State A: 12V (11V - 13V)
    // State B: 9V (8V - 10V)
    // State C: 6V (5V - 7V)
    // State D: 3V (2V - 4V)
    // State E/F: < 1V or 0V
    
    if (voltage_v > 10.5f) return CP_STATE_A;
    if (voltage_v > 7.5f)  return CP_STATE_B;
    if (voltage_v > 4.5f)  return CP_STATE_C;
    if (voltage_v > 1.5f)  return CP_STATE_D;
    
    return CP_STATE_E; // or F
}

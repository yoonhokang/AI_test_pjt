/**
 * @file    control_pilot.h
 * @brief   IEC 61851 Control Pilot (CP) Driver
 * @author  Antigravity
 * @date    2026-01-31
 */

#ifndef MODULES_CP_CONTROL_PILOT_H_
#define MODULES_CP_CONTROL_PILOT_H_

#include "main.h"

// CP State Definitions (IEC 61851)
typedef enum
{
    CP_STATE_A = 0, // Not Connected (12V)
    CP_STATE_B,     // Connected (9V)
    CP_STATE_C,     // Charging (6V)
    CP_STATE_D,     // Ventilation (3V)
    CP_STATE_E,     // Error (0V)
    CP_STATE_F      // Fault (-12V) - Not detectable with single ended ADC usually, mostly seen as 0V or different
} CP_State_t;

/**
 * @brief Initialize Control Pilot Drivers (PWM & ADC)
 *        Configures TIM1 for 1kHz output.
 */
void CP_Init(void);

/**
 * @brief Set Control Pilot PWM Duty Cycle
 * @param duty_percent Duty Cycle (0.0 to 100.0)
 *                     - 100.0: DC +12V
 *                     - 0.0:   DC -12V (or 0V/Error)
 *                     - 5.0 - 96.0: Active PWM
 */
void CP_SetPWM(float duty_percent);

/**
 * @brief Read Control Pilot Voltage
 * @return float Voltage in Volts (0.0 - 12.0+)
 */
float CP_ReadVoltage(void);

/**
 * @brief Determine IEC 61851 State from Voltage
 * @param voltage_v Measured Voltage
 * @return CP_State_t Detected State
 */
CP_State_t CP_GetStateFromVoltage(float voltage_v);

#endif /* MODULES_CP_CONTROL_PILOT_H_ */

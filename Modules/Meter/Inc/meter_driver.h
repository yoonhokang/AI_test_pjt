/**
 * @file    meter_driver.h
 * @brief   Power Meter & Sensor Driver (Voltage, Current, Temp)
 * @author  Antigravity
 * @date    2026-01-31
 */

#ifndef MODULES_METER_METER_DRIVER_H_
#define MODULES_METER_METER_DRIVER_H_

#include "main.h"

// Configuration: Virtual or Real
// Configuration: Virtual or Real
// If hardware not connected, use Simulation Mode
#define METER_SIMULATION_MODE  0 // Set to 0 to enable Modbus Logic
#define METER_USE_MODBUS       1

// Modbus Config
#define METER_MODBUS_ADDR      1
#define METER_REG_VOLTAGE      0x0010
#define METER_REG_CURRENT      0x0012


// Sim helper
void Meter_Sim_SetCurrent(float amps);

/**
 * @brief Initialize Meter Driver (ADC)
 */
void Meter_Init(void);

/**
 * @brief Process Meter Data (Polling)
 */
void Meter_Process(void);

/**
 * @brief Read AC Voltage (RMS)
 * @return Voltage in Volts (e.g., 220.5)
 */
float Meter_ReadVoltage(void);

/**
 * @brief Read AC Current (RMS)
 * @return Current in Amps (e.g., 32.1)
 */
float Meter_ReadCurrent(void);

/**
 * @brief Read Temperature
 * @return Temperature in Celsius (e.g., 45.0)
 */
float Meter_ReadTemperature(void);

/**
 * @brief Read Power (Calculated)
 * @return Power in Watts
 */
float Meter_ReadPower(void);

/**
 * @brief Read Energy (Accumulated)
 * @return Energy in kWh
 */
float Meter_ReadEnergy(void);

/**
 * @brief UART Rx Complete Callback (Hook from HAL_UART_RxCpltCallback)
 * @param huart UART Handle
 */
void Meter_RxCpltCallback(UART_HandleTypeDef *huart);


#endif /* MODULES_METER_METER_DRIVER_H_ */

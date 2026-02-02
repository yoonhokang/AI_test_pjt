/**
 * @file    safety_monitor.c
 * @brief   Safety Monitor Implementation
 */

#include "safety_monitor.h"
#include <stdio.h>

void Safety_Init(void)
{
    // GPIOs are already inited by MX_GPIO_Init in main.c
    // Just print status
    printf("[Safety] Monitor Initialized. Checking E-Stop (PC13)...\r\n");
    
    if (Safety_Check() != SAFETY_OK)
    {
        printf("[Safety] WARNING: E-Stop Active at Boot!\r\n");
    }
}

Safety_Status_t Safety_Check(void)
{
    // Emergency Stop is on PC13.
    // Nucleo User Button (Blue) is PC13.
    // Schematic: PC13 pulled LOW by button press? Or High?
    // Nucleo-64 (MB1136): PC13 is connected to B1 (USER). 
    // Usually Active LOW (Pressed = Low, Released = High) with External Pull-up?
    // Wait, STM32G4 Nucleo User Manual:
    // B1 USER: Connected to PC13. "When the button is pressed the logic state is 0, otherwise 1".
    // So Active Low.
    
    GPIO_PinState estop_state = HAL_GPIO_ReadPin(Emergency_Stop_GPIO_Port, Emergency_Stop_Pin);
    
    if (estop_state == GPIO_PIN_RESET) // Logic 0 = Pressed
    {
        return SAFETY_FAULT_ESTOP;
    }
    
    return SAFETY_OK;
}

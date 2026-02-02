#include "app_main.h"
#include "main.h"
#include "usart.h" 
#include "fdcan.h"
#include "cmsis_os.h"
#include "uart_driver.h"
#include "can_driver.h"
#include "control_pilot.h"
#include "relay_driver.h"
#include "safety_monitor.h"
#include "secc_driver.h"
#include "meter_driver.h"
#include "watchdog_driver.h"
#include "config_manager.h"
#include "infy_power.h"
#include "ocpp_app.h"
#include "cli.h"
#include "app_state.h"
#include <stdio.h>

void App_Main(void)
{
    // Initialize UART CLI (Targeting USART2 - Virtual COM)
    UART_CLI_Init(&huart2);

    // Initialize FDCAN Driver (FDCAN1)
    CAN_Driver_Init(&hfdcan1);

    // Initialize Control Pilot (PWM/ADC)
    CP_Init();

    // Initialize Relays (Safe Open)
    Relay_Init();

    // Initialize Safety Monitor
    Safety_Init();
    
    // Initialize SECC (CAN Protocol)
    SECC_Init(&hfdcan1);
    CAN_SetRxCallback(SECC_RxHandler);

    // Initialize Command Line Interface
    CLI_Init();
    SECC_Init(&hfdcan1);

    // Initialize Watchdog
    Watchdog_Init();

    // Initialize Config
    Config_Init();

    // Initialize Power Module
    Infy_Init(&hfdcan1);

    // Initialize OCPP
    OCPP_Init();

    // Initialize State Machine

    // Initialize State Machine
    StateMachine_Init();

    printf("[App] Initialization Complete. Entering Loop...\r\n");

    // --- Main Loop ---
    while (1)
    {
        // Refresh Watchdog
        Watchdog_Refresh();
        
        // Process OCPP
        OCPP_Process();

        HAL_Delay(10); // Sleep 10ms
        CLI_Process();

        // 2. Execute State Machine Logic
        // 2. Execute State Machine Logic
        StateMachine_Loop();

        // 3. SECC Communication (50ms interval)
        static uint32_t last_secc_tx = 0;
        if ((HAL_GetTick() - last_secc_tx) >= 50)
        {
            float cp_v = CP_ReadVoltage();
            // PWM Duty: Need getter, for now hardcode logic or 0
            // Relay:
            uint8_t relays = Relay_GetState();
            // Fault: 
            // 0 for now
            SECC_TxStatus(cp_v, 0, relays, 0); 
            
            // 4. Meter Values (Every 4th cycle = 200ms)
            static uint8_t meter_prescaler = 0;
            if (++meter_prescaler >= 4)
            {
                SECC_TxMeter(Meter_ReadVoltage(), Meter_ReadCurrent(), Meter_ReadTemperature());
                meter_prescaler = 0;
            }
            
            last_secc_tx = HAL_GetTick();
        }

        osDelay(10); // Maintain OS responsiveness
    }
}

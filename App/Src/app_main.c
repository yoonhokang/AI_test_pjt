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
#include "imd_driver.h"
#include "ocpp_app.h"
#include "cli.h"
#include "app_state.h"
#include "logger.h" // For Async Logging
#include <stdio.h>


void App_Init(void)
{
    // Initialize UART CLI (Targeting USART2 - Virtual COM)
    UART_CLI_Init(&huart2);

    // Initialize FDCAN Driver (FDCAN1 - SECC)
    CAN_Driver_Init(&hfdcan1);

    // Initialize FDCAN Driver (FDCAN2 - Power Modules)
    CAN_Driver_Init(&hfdcan2);

    // Initialize FDCAN Driver (FDCAN3 - IMD)
    CAN_Driver_Init(&hfdcan3);

    // Initialize Control Pilot (PWM/ADC)
    CP_Init();

    // Initialize Relays (Safe Open)
    Relay_Init();

    // Initialize Safety Monitor
    Safety_Init();
    
    // Initialize SECC (CAN Protocol - FDCAN1)
    SECC_Init(&hfdcan1);
    
    // Register Dispatcher for CAN Rx
    void App_RxDispatcher(uint32_t id, uint8_t *data, uint8_t len); // Forward declaration
    CAN_SetRxCallback(App_RxDispatcher);

    // Initialize Command Line Interface
    CLI_Init();

    // Initialize Watchdog
    Watchdog_Init();

    // Initialize Config
    Config_Init();

    // Initialize Power Module (FDCAN2)
    Infy_Init(&hfdcan2);

    // Initialize IMD (FDCAN3 - Independent Bus)
    IMD_Init(&hfdcan3);

    // Initialize OCPP
    OCPP_Init();

    // Initialize State Machine
    StateMachine_Init();

    Logger_Print("[App] Initialization Complete. Tasks Starting...\r\n");
}


void App_ControlLoop(void)
{
    // --- Control Loop (High Priority, Non-Blocking) ---
    while (1)
    {
        // 1. Critical Safety & Watchdog
        Watchdog_Refresh(); // Refresh here (Highest Priority)
        
        // 2. CLI Process (Quick Check)
        CLI_Process();

        // 3. Execute State Machine Logic (Safety Check Inside)
        StateMachine_Loop();

        // 4. SECC Communication (50ms interval)
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
            
            // 4. Meter Values (Every 4th cycle = 200ms) -> Too fast for OCPP
            // Let's use a separate counter for OCPP (e.g. 5 seconds)
            static uint32_t last_ocpp_meter = 0;
            if ((HAL_GetTick() - last_ocpp_meter) >= 5000)
            {
                // Real Meter Values
                OCPP_SendMeterValues(1, Meter_ReadEnergy(), Meter_ReadPower(), (int)Meter_ReadTemperature());

                last_ocpp_meter = HAL_GetTick();
            }

            static uint8_t meter_prescaler = 0;
            if (++meter_prescaler >= 4)
            {
                SECC_TxMeter(Meter_ReadVoltage(), Meter_ReadCurrent(), Meter_ReadTemperature());
                meter_prescaler = 0;
            }
            
            last_secc_tx = HAL_GetTick();
        }

        // Periodic Meter Processing (Modbus State Machine - Call Frequently)
        Meter_Process();


        osDelay(10); // Maintain OS responsiveness (10ms tick)
    }
}

void App_OCPPLoop(void)
{
    // --- OCPP Loop (Normal Priority, Can Block) ---
    while (1)
    {
        // Process OCPP (TCP/TLS connect may block here)
        OCPP_Process();

        // Short delay to yield if idle
        osDelay(10); 
    }
}

/**
 * @brief Top-level CAN Rx Dispatcher
 * @note  Routes messages to specific modules based on CAN ID.
 */
void App_RxDispatcher(uint32_t id, uint8_t *data, uint8_t len)
{
    // SECC Range Check (Assumed Single ID for now, but safer to dispatch specific ID)
    if (id == SECC_CAN_ID_RX_CMD) 
    {
        SECC_RxHandler(id, data, len);
    }
    // Infy Power Module Range Check
    // Range: 0x18005001 ~ 0x1800500A (Base + 1..Max)
    else if (id > INFY_CAN_ID_CONTROL_BASE && id <= (INFY_CAN_ID_CONTROL_BASE + INFY_MAX_MODULES))
    {
        Infy_RxHandler(id, data, len);
    }
    // IMD Message Check
    else if (id == IMD_CAN_ID_TX_INFO)
    {
        IMD_RxHandler(id, data, len);
    }
}

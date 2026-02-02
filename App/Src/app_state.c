/**
 * @file    app_state.c
 * @brief   Core EVSE State Machine Implementation
 */

#include "app_state.h"
#include "main.h"       // For LED Pins and HAL_GetTick
#include "secc_driver.h"
#include "relay_driver.h"
#include "control_pilot.h"
#include "safety_monitor.h"
#include "infy_power.h"
#include "ocpp_app.h" // For Auto-Stop
#include "meter_driver.h" // For Welding Check
#include <stdio.h>      // For printf
#include <math.h>       // For fabsf

// Internal Variables
static EVSE_State_t current_state = STATE_BOOT;
static uint32_t last_led_tick = 0;
static uint32_t led_interval = 500; // Default blink interval (ms)
static uint32_t precharge_tick = 0;

// Charging Sequence Variables - REMOVED (AC/Standalone Logic Deleted) 

void StateMachine_Init(void)
{
    printf("[State] Initializing... Set to BOOT\r\n");
    // Ensure relays are open
    Relay_SetMain(false);
    Relay_SetPrecharge(false);
    
    StateMachine_SetState(STATE_BOOT);
}

bool StateMachine_TryClearFault(void)
{
    if (Safety_Check() == SAFETY_OK)
    {
        printf("[State] Fault Cleared by User.\r\n");
        StateMachine_SetState(STATE_STANDBY);
        
        // Notify OCPP Status
        // OCPP_SendStatusNotification(1, "Available", "NoError"); 
        return true;
    }
    else
    {
        printf("[State] Cannot Clear Fault! Safety condition still active.\r\n");
        return false;
    }
}

bool StateMachine_RemoteStart(const char* id_tag)
{
    if (current_state == STATE_CONNECTED)
    {
        printf("[State] Remote Start Accepted (Tag: %s)\r\n", id_tag);
        // In a real system, we might need to authorize first or check EV Ready
        // For now, simulate Auth Success and move to Charging
        
        // Trigger EVSE to Allow Power
        // But in this FW, we follow SECC. 
        // If we are master, we tell SECC to charge. 
        // Since we are slave to SECC, we can only 'enable' our side.
        
        // Logic: Send "Authorize" to SECC? Or just change state?
        // Let's assume we proceed to Pre-Charge State to check safety first
        StateMachine_SetState(STATE_PRECHARGE);
        return true;
    }
    
    printf("[State] Remote Start Rejected. Current State: %s\r\n", StateMachine_GetStateName(current_state));
    return false;
}

bool StateMachine_RemoteStop(void)
{
    if (current_state == STATE_CHARGING)
    {
        printf("[State] Remote Stop Received. Stopping...\r\n");
        StateMachine_SetState(STATE_CONNECTED); // Return to Connected (B/C) -> Relays Open
        
        // [FIX] Send StopTransaction to Backend
        OCPP_SendStopTransaction();
        return true;
    }
    return false;
}

void StateMachine_SetState(EVSE_State_t new_state)
{
    if (current_state != new_state)
    {
        printf("[State] Transition: %s -> %s\r\n", 
               StateMachine_GetStateName(current_state), 
               StateMachine_GetStateName(new_state));
        
        // On Exit Actions
        if (current_state == STATE_CHARGING)
        {
             // Safely open relays when leaving charging state
             printf("[Seq] Stopping Charge -> OPEN Relays\r\n");
             Relay_SetMain(false);
             Relay_SetPrecharge(false);
        }

        current_state = new_state;
        
        // On Entry Actions
        switch (current_state)
        {
            case STATE_BOOT:
                led_interval = 0; // Solid On logic
                CP_SetPWM(100.0f);
                break;
            case STATE_STANDBY: // State A
                led_interval = 1000; // Slow Blink
                CP_SetPWM(100.0f);   // 12V DC
                break;
            case STATE_CONNECTED: // State B
                led_interval = 500;  // Medium Blink
                CP_SetPWM(53.3f);    // PWM 53% (Approx 32A)
                break;
            case STATE_PRECHARGE:
                led_interval = 200; // Fast Blink (Preparing)
                // Entry Action: Check Welding
                if (Meter_ReadVoltage() > 50.0f)
                {
                    printf("[Safety] Welding Detected on Entry! V > 50V\r\n");
                }
                precharge_tick = HAL_GetTick();
                Relay_SetPrecharge(true); // Virtual Log (No physical relay)
                
                // [Soft-Start] Set Target Voltage
                // Priority: Measured Voltage > SECC Request > Default
                float target_v = secc_control.ev_target_voltage;
                if (Meter_ReadVoltage() > 20.0f) target_v = Meter_ReadVoltage(); 
                if (target_v < 10.0f) target_v = 350.0f; // Default if 0 to prevent 0V start
                
                Infy_SetOutput(target_v, 2.0f, true); // 2A Current Limit for Soft Start
                printf("[Seq] Pre-Charge Soft-Start. Target: %.1fV\r\n", target_v);
                break;
            case STATE_CHARGING: // State C
                led_interval = 100; // Very Fast Blink
                // PWM remains 53%
                // PWM remains 53%
                break;
            case STATE_FAULT:
                led_interval = 200; // SOS Pattern
                CP_SetPWM(0.0f);    // 0V or -12V (Error)
                Relay_SetMain(false);      // Safety
                Relay_SetPrecharge(false); // Safety
                break;
        }
        
        // Reset LED timer
        last_led_tick = HAL_GetTick();
    }
}

#include "secc_driver.h"

void StateMachine_Loop(void)
{
    // 0. Safety Check (Highest Priority)
    if (current_state != STATE_FAULT)
    {
        if (Safety_Check() != SAFETY_OK)
        {
            printf("[Safety] CRITICAL FAULT DETECTED!\r\n");
            StateMachine_SetState(STATE_FAULT);
            return; // Exit loop
        }
        
        // [New] Welding Detection Check
        if (current_state != STATE_CHARGING && current_state != STATE_FAULT)
        {
             static uint32_t high_v_tick = 0;

             // If Relays are OPEN but Voltage is High (>60V)
             if ((Relay_GetState() == 0) && (Meter_ReadVoltage() > 60.0f))
             {
                 if (high_v_tick == 0) high_v_tick = HAL_GetTick();
                 
                 if (HAL_GetTick() - high_v_tick > 2000) // > 2 Seconds
                 {
                      printf("[Safety] WELDING DETECTED! Voltage: %.1fV\r\n", Meter_ReadVoltage());
                      StateMachine_SetState(STATE_FAULT);
                      return;
                 }
             }
             else
             {
                 high_v_tick = 0; // Reset
             }
        }
    }

    // 1. Periodic Actions (LED Blinking)
    uint32_t now = HAL_GetTick();
    if (led_interval > 0)
    {
        if ((now - last_led_tick) >= led_interval)
        {
            HAL_GPIO_TogglePin(Status_LED_GPIO_Port, Status_LED_Pin);
            last_led_tick = now;
        }
    }
    else
    {
        // Solid On (Boot)
        if (current_state == STATE_BOOT)
        {
            HAL_GPIO_WritePin(Status_LED_GPIO_Port, Status_LED_Pin, GPIO_PIN_SET);
        }
    }

    // 2. Control Logic (SECC vs Standalone)
    if (SECC_IsConnected())
    {
        // --- Remote Control Mode ---
        // 1. PWM Control
        CP_SetPWM((float)secc_control.target_pwm_duty);
        
        // 2. Pre-Charge Sequence Logic
        if (current_state == STATE_PRECHARGE)
        {
            // Dynamic Target Adjustment
            float target_v = secc_control.ev_target_voltage;
            float meter_v = Meter_ReadVoltage();
            if (meter_v > 20.0f) target_v = meter_v;
            if (target_v < 10.0f) target_v = 350.0f; // Safety Default

            // Keep updating Output
            Infy_SetOutput(target_v, 2.0f, true);

            // Check Voltage Match
            const Infy_SystemStatus_t* pwr_status = Infy_GetSystemStatus();
            float internal_v = pwr_status->total_voltage;

            // Debug Log every 500ms
            if ((HAL_GetTick() % 500) == 0)
            {
                 printf("[Seq] Pre-Charge: Int=%.1fV, Meter=%.1fV, Tgt=%.1fV\r\n", internal_v, meter_v, target_v);
            }
            
            // Sync Condition: Voltage Diff < 20V AND Min Time > 1s
            if (fabsf(internal_v - target_v) < 20.0f)
            {
                if (HAL_GetTick() - precharge_tick > 1000)
                {
                    printf("[Seq] Voltage Matched. Transition to CHARGING.\r\n");
                    StateMachine_SetState(STATE_CHARGING);
                }
            }
            // Timeout Check (10 Seconds)
            else if (HAL_GetTick() - precharge_tick > 10000)
            {
                printf("[Seq] Pre-Charge Timeout! Failed to match voltage.\r\n");
                StateMachine_SetState(STATE_FAULT);
            }
        }

        // 3. Relay & Power Control (Remote)
        // [FIX] Only allow power if SECC requests IT AND we are logically in CHARGING state.
        // This ensures User App Stop (which sets state to CONNECTED) overrides SECC.
        if (secc_control.allow_power && current_state == STATE_CHARGING)
        {
            // Simple Safe Close Sequence
            if ((Relay_GetState() & 0x01) == 0) // Main Open
            {
                 // DC Sequence: Voltage Match first? 
                 // Now handled by Pre-Charge State. Direct Close here is failsafe.
                 Relay_SetMain(true);
                 Relay_SetPrecharge(false); // Open Pre-Charge
            }
            
            // Set Power Module Output (Remote Control)
            float target_v = secc_control.ev_target_voltage;
            float target_i = secc_control.ev_max_current;
            if (target_v < 10.0f) target_v = 350.0f; // Default if 0
            
            Infy_SetOutput(target_v, target_i, true);
        }
        else if (current_state != STATE_PRECHARGE) // Keep Pre-charge active if in that state
        {
            // Open Relays & Disable Power
            Infy_SetOutput(0.0f, 0.0f, false);
            Relay_SetMain(false);
            Relay_SetPrecharge(false);

            // [Auto-Stop] Check if we were charging but EV stopped requesting power
            if (current_state == STATE_CHARGING)
            {
                printf("[State] EV Stopped Charge (AllowPower=0) -> Auto Stop Transaction\r\n");
                // 1. Transition State (This opens relays again comfortably)
                StateMachine_SetState(STATE_CONNECTED);
                
                // 2. Trigger OCPP Stop
                OCPP_SendStopTransaction();
            }
        }
        
        // Skip Standalone State Logic
        return;
    }

    // 3. Standalone Mode (Removed per Design Requirement)
    // If SECC is not connected, ensure system is in safe state
    CP_SetPWM(100.0f);    // 12V (State A)
    Relay_SetMain(false); // Open Relays
    Relay_SetPrecharge(false);
    Infy_SetOutput(0.0f, 0.0f, false); // Disable Power Module
}

const char* StateMachine_GetStateName(EVSE_State_t state)
{
    switch (state)
    {
        case STATE_BOOT: return "BOOT";
        case STATE_STANDBY: return "STANDBY";
        case STATE_CONNECTED: return "CONNECTED";
        case STATE_PRECHARGE: return "PRECHARGE";
        case STATE_CHARGING: return "CHARGING";
        case STATE_FAULT: return "FAULT";
        default: return "UNKNOWN";
    }
}

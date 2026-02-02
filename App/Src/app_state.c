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
#include <stdio.h>      // For printf

// Internal Variables
static EVSE_State_t current_state = STATE_BOOT;
static uint32_t last_led_tick = 0;
static uint32_t led_interval = 500; // Default blink interval (ms)

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
        return true;
    }
    else
    {
        printf("[State] Cannot Clear Fault! Safety condition still active.\r\n");
        return false;
    }
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
            case STATE_CHARGING: // State C
                led_interval = 100; // Fast Blink
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

// ... (other includes)

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
        
        // 2. Relay & Power Control (Remote)
        if (secc_control.allow_power)
        {
            // Simple Safe Close Sequence
            if ((Relay_GetState() & 0x01) == 0) // Main Open
            {
                 // DC Sequence: Voltage Match first? 
                 // For now, assume remote handles sequencing or direct close
                 Relay_SetMain(true);
            }
            
            // Set Power Module Output (Remote Control)
            float target_v = secc_control.ev_target_voltage;
            float target_i = secc_control.ev_max_current;
            if (target_v < 10.0f) target_v = 350.0f; // Default if 0
            
            Infy_SetOutput(target_v, target_i, true);
        }
        else
        {
            // Open Relays & Disable Power
            Infy_SetOutput(0.0f, 0.0f, false);
            Relay_SetMain(false);
        }
        
        // Skip Standalone State Logic
        return;
    }

    // 3. Standalone Mode (Removed per Design Requirement)
    // If SECC is not connected, ensure system is in safe state
    CP_SetPWM(100.0f);    // 12V (State A)
    Relay_SetMain(false); // Open Relays
    Infy_SetOutput(0.0f, 0.0f, false); // Disable Power Module
}

const char* StateMachine_GetStateName(EVSE_State_t state)
{
    switch (state)
    {
        case STATE_BOOT: return "BOOT";
        case STATE_STANDBY: return "STANDBY";
        case STATE_CONNECTED: return "CONNECTED";
        case STATE_CHARGING: return "CHARGING";
        case STATE_FAULT: return "FAULT";
        default: return "UNKNOWN";
    }
}

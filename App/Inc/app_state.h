/**
 * @file    app_state.h
 * @brief   Core EVSE State Machine Definitions
 * @author  Antigravity
 * @date    2026-01-31
 */

#ifndef APP_APP_STATE_H_
#define APP_APP_STATE_H_

#include "main.h" // For HAL types if needed
#include <stdbool.h>

// System States
typedef enum
{
    STATE_BOOT = 0,
    STATE_STANDBY,
    STATE_CONNECTED,
    STATE_PRECHARGE,
    STATE_CHARGING,
    STATE_FAULT,
} EVSE_State_t;

/**
 * @brief Initialize the State Machine
 */
void StateMachine_Init(void);

/**
 * @brief Main Loop for the State Machine (Non-blocking)
 */
void StateMachine_Loop(void);

/**
 * @brief Force a state transition (Debug/CLI use)
 * @param new_state Target state
 */
void StateMachine_SetState(EVSE_State_t new_state);

/**
 * @brief Attempt to clear FAULT state.
 *        Checks safety conditions first.
 * @return true if cleared, false if safety still active
 */
bool StateMachine_TryClearFault(void);

/**
 * @brief Get current state name as string
 * @return const char* State name
 */
const char* StateMachine_GetStateName(EVSE_State_t state);

/**
 * @brief Handle Remote Start Transaction from OCPP
 * @param id_tag Authorization Tag
 * @return true if accepted, false if rejected
 */
bool StateMachine_RemoteStart(const char* id_tag);

/**
 * @brief Handle Remote Stop Transaction from OCPP
 * @return true if accepted
 */
bool StateMachine_RemoteStop(void);

#endif /* APP_APP_STATE_H_ */

/**
 * @file    cli.c
 * @brief   Command Line Interface Implementation
 */

#include "cli.h"
#include "uart_driver.h"
#include "main.h" // For HAL_NVIC_SystemReset
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define CLI_MAX_CMD_LENGTH 64

static char cli_buffer[CLI_MAX_CMD_LENGTH];
static uint8_t cli_buffer_index = 0;

// Function Prototypes for Commands
static void Cmd_Help(void);
static void Cmd_Status(void);
static void Cmd_Reset(void);
static void Cmd_CanTest(void);
static void Cmd_StateBoot(void);
static void Cmd_StateStandby(void);
static void Cmd_StateCharge(void);
static void Cmd_StateFault(void);
static void Cmd_CPStatus(void);
static void Cmd_PWMTest(void);
static void Cmd_RelayTest(void);
static void Cmd_MeterStatus(void);
static void Cmd_MeterSet(void);
// Config Commands
#include "config_manager.h"

static void Cmd_ConfigSave(void);
static void Cmd_ConfigShow(void);
static void Cmd_PowerTest(void);
static void Cmd_OCPPStart(void);
static void Cmd_OCPPStop(void);
static void Cmd_FaultClear(void);

// Command Table Structure
typedef struct
{
    const char *cmd_name;
    const char *help_text;
    void (*handler)(void);
} CLI_Command_t;

// Command List
static const CLI_Command_t cli_commands[] = {
    {"help",   "Show this help message", Cmd_Help},
    {"status", "Show system status",     Cmd_Status},
    {"reset",  "Reset the microcontroller",   Cmd_Reset},
    {"can_test", "Send Test CAN Frame (ID 0x123)", Cmd_CanTest},
    {"state_boot", "Force State: BOOT", Cmd_StateBoot},
    {"state_wait", "Force State: STANDBY", Cmd_StateStandby},
    {"state_charge", "Force State: CHARGING", Cmd_StateCharge},
    {"state_fault", "Force State: FAULT", Cmd_StateFault},
    {"cp_status", "Show Control Pilot Voltage/State", Cmd_CPStatus},
    {"pwm_test", "Cycle PWM (100 -> 5 -> 53 -> 0)", Cmd_PWMTest},
    {"relay_test", "Cycle Relays (Safe -> Pre -> Main -> Dual)", Cmd_RelayTest},
    {"meter_status", "Show V/I/P/T", Cmd_MeterStatus},
    {"meter_test", "Cycle Sim Current (0->16->32)", Cmd_MeterSet},
    {"config_save", "Save Config to Flash", Cmd_ConfigSave},
    {"config_show", "Show Config Data", Cmd_ConfigShow},
    {"power_test",  "Toggle 400V Output (Sim)", Cmd_PowerTest},
    {"ocpp_start",  "Send StartTransaction",    Cmd_OCPPStart},
    {"ocpp_stop",   "Send StopTransaction",     Cmd_OCPPStop},
    {"fault_clear", "Try to clear FAULT state", Cmd_FaultClear},
    // Add new commands here
    {NULL, NULL, NULL} // Terminator
};

void CLI_Init(void)
{
    cli_buffer_index = 0;
    memset(cli_buffer, 0, CLI_MAX_CMD_LENGTH);
    printf("\r\n[CLI] Ready. Type 'help' for commands.\r\n> ");
}

static void CLI_ExecuteCommand(char *cmd_line)
{
    // Trim leading spaces
    char *start = cmd_line;
    while (*start && isspace((unsigned char)*start)) start++;

    if (*start == '\0') return; // Empty command

    // Find the end of the command (space or null)
    char *end = start;
    while (*end && !isspace((unsigned char)*end)) end++;
    
    // Null-terminate the command string for comparison
    char saved_char = *end;
    *end = '\0';

// Search command in table
    const CLI_Command_t *cmd = cli_commands;
    bool found = false;
    while (cmd->cmd_name != NULL)
    {
        // Simple case-insensitive match
        const char *s1 = start;
        const char *s2 = cmd->cmd_name;
        int match = 1;
        while (*s1 && *s2)
        {
            if (tolower((unsigned char)*s1) != tolower((unsigned char)*s2))
            {
                match = 0;
                break;
            }
            s1++;
            s2++;
        }
        if (match && *s1 == '\0' && *s2 == '\0') // Ensure same length
        {
            *end = saved_char; 
            cmd->handler();
            found = true;
            break;
        }
        cmd++;
    }

    if (!found)
    {
        printf("Unknown command: '%s'\r\n", start);
    }
}

void CLI_Process(void)
{
    uint8_t rx_byte;

    // Process all available bytes from UART
    while (UART_CLI_Read(&rx_byte)) 
    {
        // Local Echo Logic, buffering, etc.
        
        if (rx_byte == '\r' || rx_byte == '\n')
        {
            printf("\r\n"); // New line
            if (cli_buffer_index > 0)
            {
                cli_buffer[cli_buffer_index] = '\0';
                CLI_ExecuteCommand(cli_buffer);
                cli_buffer_index = 0;
            }
            printf("> ");
        }
        else if (rx_byte == '\b' || rx_byte == 0x7F) // Backspace
        {
            if (cli_buffer_index > 0)
            {
                cli_buffer_index--;
                printf("\b \b"); // Visual backspace
            }
        }
        else if (cli_buffer_index < CLI_MAX_CMD_LENGTH - 1)
        {
            if (isprint(rx_byte))
            {
                cli_buffer[cli_buffer_index++] = (char)rx_byte;
                printf("%c", rx_byte); // Local echo
            }
        }
    }
}

// --- Command Handlers ---

static void Cmd_Help(void)
{
    printf("\r\n--- Available Commands ---\r\n");
    const CLI_Command_t *cmd = cli_commands;
    while (cmd->cmd_name != NULL)
    {
        printf(" %-10s : %s\r\n", cmd->cmd_name, cmd->help_text);
        cmd++;
    }
}

static void Cmd_Status(void)
{
    printf("\r\n--- System Status ---\r\n");
    printf(" Tick: %lu ms\r\n", HAL_GetTick());
    // Add more status info here later (e.g. FreeRTOS heap, tasks)
}

static void Cmd_Reset(void)
{
    printf("Resetting system...\r\n");
    HAL_Delay(100);
    HAL_NVIC_SystemReset();
}

// Ensure FDCAN handle is accessible
#include "fdcan.h" 
#include "can_driver.h"
#include "app_state.h"

static void Cmd_CanTest(void)
{
    uint8_t test_data[] = {0xDE, 0xAD, 0xBE, 0xEF, 0x01, 0x02, 0x03, 0x04};
    printf("Sending CAN Frame ID: 0x123... ");
    
    if (CAN_Transmit(&hfdcan1, 0x123, test_data, 8))
    {
        printf("Success\r\n");
    }
    else
    {
        printf("Failed (Tx FIFO Full?)\r\n");
    }
}


static void Cmd_StateBoot(void) { StateMachine_SetState(STATE_BOOT); }
static void Cmd_StateStandby(void) { StateMachine_SetState(STATE_STANDBY); }
static void Cmd_StateCharge(void) { StateMachine_SetState(STATE_CHARGING); }
static void Cmd_StateFault(void) { StateMachine_SetState(STATE_FAULT); }

// Control Pilot Commands
#include "control_pilot.h"
#include <stdlib.h> // for atof if needed, but we keep it simple

static void Cmd_CPStatus(void)
{
    float volts = CP_ReadVoltage();
    CP_State_t state = CP_GetStateFromVoltage(volts);
    const char *state_str = "?";
    
    switch(state)
    {
        case CP_STATE_A: state_str = "A (Not Conn)"; break;
        case CP_STATE_B: state_str = "B (Connected)"; break;
        case CP_STATE_C: state_str = "C (Charging)"; break;
        case CP_STATE_D: state_str = "D (Vent)"; break;
        case CP_STATE_E: state_str = "E (Error)"; break;
        default: break;
    }
    
    printf("[CP] Voltage: %.2f V, State: %s\r\n", volts, state_str);
}

static void Cmd_PWMTest(void)
{
    // Simple toggle for testing
    static int mode = 0;
    mode = (mode + 1) % 4;
    
    float duty = 100.0f;
    switch(mode) {
        case 0: duty = 100.0f; break; // DC 12V
        case 1: duty = 5.0f; break;   // Handshake
        case 2: duty = 53.3f; break;  // 32A Charging
        case 3: duty = 0.0f; break;   // Off/-12V
    }
    
    CP_SetPWM(duty);
    printf("[CP] PWM Set to %.1f%%\r\n", duty);
}

// Relay Commands
#include "relay_driver.h"

static void Cmd_RelayTest(void)
{
    static int stage = 0;
    stage = (stage + 1) % 4;
    
    switch(stage)
    {
        case 1:
            printf("[Relay] Precharge CLOSED\r\n");
            Relay_SetPrecharge(true);
            Relay_SetMain(false);
            break;
        case 2:
            printf("[Relay] Main CLOSED, Precharge OPEN\r\n");
            Relay_SetMain(true);
            Relay_SetPrecharge(false);
            break;
        case 3:
            printf("[Relay] Main CLOSED, Precharge CLOSED (Dual)\r\n");
            Relay_SetMain(true);
            Relay_SetPrecharge(true);
            break;
        case 0:
        default:
            printf("[Relay] ALL OPEN (Safe)\r\n");
            Relay_SetMain(false);
            Relay_SetPrecharge(false);
            break;
    }
}

// Meter Commands
#include "meter_driver.h"

static void Cmd_MeterStatus(void)
{
    printf("[Meter] %.1f V, %.1f A, %.1f W, %.1f C\r\n",
           Meter_ReadVoltage(),
           Meter_ReadCurrent(),
           Meter_ReadPower(),
           Meter_ReadTemperature());
}

static void Cmd_MeterSet(void)
{
    static int level = 0;
    level = (level + 1) % 3;
    float target = 0.0f;
    if (level == 1) target = 16.0f;
    if (level == 2) target = 32.0f;
    
    Meter_Sim_SetCurrent(target);
    printf("[Meter] Sim Current Set to %.1f A\r\n", target);
}

// Config Commands
#include "config_manager.h"

static void Cmd_ConfigSave(void)
{
    Config_Save();
}

static void Cmd_ConfigShow(void)
{
    SystemConfig_t *cfg = Config_Get();
    printf("[Config] Max Current: %.1f A, Fault: %d, Boots: %lu\r\n",
           cfg->max_current_a, cfg->last_fault_code, cfg->boot_count);
}

static void Cmd_FaultClear(void)
{
    StateMachine_TryClearFault();
}

// Power Module Test
#include "infy_power.h"
static void Cmd_PowerTest(void)
{
    static bool on = false;
    on = !on;
    
    if (on)
    {
        printf("[Power] Test Starting... Targeting 400V @ 10A\r\n");
        // Example: Set 400V, 10A, Enabled
        Infy_SetOutput(400.0f, 10.0f, true);
    }
    else
    {
        printf("[Power] Test Stopping... Targeting 0V (Disable)\r\n");
        // Safe Shutdown: Voltage 0, Current 0, Disable
        Infy_SetOutput(0.0f, 0.0f, false);
    }
    
    // Readback Immediate Status
    // Readback Immediate Status
    const Infy_SystemStatus_t *st = Infy_GetSystemStatus();
    printf("[Power] Status: %.1f V, %.1f A, Fault=%d\r\n", 
           st->total_voltage, st->total_current, st->system_fault);
}

// OCPP Commands
#include "ocpp_app.h"
static void Cmd_OCPPStart(void)
{
    OCPP_SendStartTransaction("RFID_1234");
}

static void Cmd_OCPPStop(void)
{
    OCPP_SendStopTransaction();
}

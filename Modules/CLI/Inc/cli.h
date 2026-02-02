/**
 * @file    cli.h
 * @brief   Command Line Interface for System Monitoring
 * @author  Antigravity
 * @date    2026-01-31
 */

#ifndef MODULES_CLI_CLI_H_
#define MODULES_CLI_CLI_H_

#include <stdint.h>

/**
 * @brief Initialize the CLI Module
 */
void CLI_Init(void);

/**
 * @brief Process pending CLI input
 *        Call this function periodically in the main loop or a low-priority task.
 */
void CLI_Process(void);

#endif /* MODULES_CLI_CLI_H_ */

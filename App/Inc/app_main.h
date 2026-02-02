#ifndef APP_MAIN_H
#define APP_MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  Application Main Entry Point.
 *         Called from DefaultTask.
 */
// Core Initialization (To be called once)
void App_Init(void);

// Task Loops
void App_ControlLoop(void); // High Priority
void App_OCPPLoop(void);    // Normal Priority

#ifdef __cplusplus
}
#endif

#endif /* APP_MAIN_H */

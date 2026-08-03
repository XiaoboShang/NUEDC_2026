#ifndef TASK_MANAGER_H
#define TASK_MANAGER_H

#include <stdint.h>

typedef enum
{
    TASK_ID_1 = 1,
    TASK_ID_2,
    TASK_ID_3,
    TASK_ID_4,
    TASK_ID_5
} task_id_t;

typedef enum
{
    TASK_STATE_SELECTING = 0,
    TASK_STATE_RUNNING,
    TASK_STATE_FINISHED,
    TASK_STATE_FAILED
} task_state_t;

/** Initialize the task selector and draw TASK1 on the OLED. */
void task_manager_init(void);

/** Process pending key events and refresh the OLED when required. */
void task_manager_process(void);

/** Return the currently selected task. */
task_id_t task_manager_get_selected(void);

/** Return the current selector/run state. */
task_state_t task_manager_get_state(void);

/**
 * Record the first completion time of the running task.
 * This function does not access the OLED and may be called from an ISR.
 */
void task_manager_finish(void);

/**
 * Record the first failure time of the running task.
 * This function does not access the OLED and may be called from an ISR.
 */
void task_manager_fail(void);

#endif

#ifndef KEY_H
#define KEY_H

#include "ti_msp_dl_config.h"

#define KEY_EVENT_NONE    (0U)
#define KEY_EVENT_SELECT  (1U << 0)
#define KEY_EVENT_CONFIRM (1U << 1)

/**
 * @brief Atomically obtain and clear all pending key events.
 * @return Bitwise OR of KEY_EVENT_SELECT and KEY_EVENT_CONFIRM.
 */
uint32_t key_take_events(void);

#endif

#include "key.h"

#define KEY_DEBOUNCE_MS (50U)

extern volatile uint32_t sys_tick_ms;

volatile uint32_t counter_1_A = 0U;
volatile uint32_t counter_2_A = 0U;

static volatile uint32_t g_key_events = KEY_EVENT_NONE;

typedef struct
{
    volatile uint32_t release_start_ms;
    volatile uint8_t armed;
    volatile uint8_t release_tracking;
} key_debounce_state_t;

static key_debounce_state_t g_select_key = {0U, 1U, 0U};
static key_debounce_state_t g_confirm_key = {0U, 1U, 0U};

static void key_record_event(
    key_debounce_state_t *key, uint32_t event)
{
    if (key->armed != 0U)
    {
        key->armed = 0U;
        key->release_tracking = 0U;
        g_key_events |= event;
    }
    else
    {
        /* Any falling edge during release debounce restarts the timer. */
        key->release_tracking = 0U;
    }
}

static void key_update_release(
    key_debounce_state_t *key, uint32_t pin, uint32_t now_ms)
{
    if (key->armed != 0U)
    {
        return;
    }

    if ((DL_GPIO_readPins(CHANGE_PORT, pin) & pin) == 0U)
    {
        key->release_tracking = 0U;
        return;
    }

    if (key->release_tracking == 0U)
    {
        key->release_start_ms = now_ms;
        key->release_tracking = 1U;
    }
    else if ((uint32_t)(now_ms - key->release_start_ms) >= KEY_DEBOUNCE_MS)
    {
        key->armed = 1U;
        key->release_tracking = 0U;
    }
}

uint32_t key_take_events(void)
{
    uint32_t interrupt_state;
    uint32_t events;
    uint32_t now_ms = sys_tick_ms;

    interrupt_state = __get_PRIMASK();
    __disable_irq();
    key_update_release(&g_select_key, CHANGE_SELECT_PIN, now_ms);
    key_update_release(&g_confirm_key, CHANGE_CONFIRM_PIN, now_ms);
    events = g_key_events;
    g_key_events = KEY_EVENT_NONE;
    if (interrupt_state == 0U)
    {
        __enable_irq();
    }

    return events;
}

void GROUP1_IRQHandler(void)
{
    DL_GPIO_IIDX pending_interrupt;

    do
    {
        pending_interrupt = DL_GPIO_getPendingInterrupt(GPIOB);
        switch (pending_interrupt)
        {
        case DC_MOTOR_BA_IIDX:
            counter_2_A++;
            break;

        case CHANGE_SELECT_IIDX:
            key_record_event(&g_select_key, KEY_EVENT_SELECT);
            break;

        case CHANGE_CONFIRM_IIDX:
            key_record_event(&g_confirm_key, KEY_EVENT_CONFIRM);
            break;

        default:
            break;
        }
    } while (pending_interrupt != DL_GPIO_IIDX_NO_INTR);

    do
    {
        pending_interrupt = DL_GPIO_getPendingInterrupt(GPIOA);
        switch (pending_interrupt)
        {
        case DC_MOTOR_AA_IIDX:
            counter_1_A++;
            break;

        default:
            break;
        }
    } while (pending_interrupt != DL_GPIO_IIDX_NO_INTR);
}

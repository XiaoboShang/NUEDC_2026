#include "task_manager.h"

#include <stdio.h>

#include "competition_tasks.h"
#include "key.h"
#include "oled.h"
#include "ti_msp_dl_config.h"

#define TASK_BLINK_INTERVAL_MS (200U)
#define TASK_TIME_INTERVAL_MS (100U)
#define TASK_TEXT_X (34U)
#define TASK_TEXT_Y (0U)
#define TASK_TEXT_SIZE (24U)
#define TASK_MARKER_LEFT_X (10U)
#define TASK_MARKER_RIGHT_X (106U)
#define TASK_TIME_X (0U)
#define TASK_TIME_Y (46U)
#define TASK_TIME_SIZE (16U)
#define TASK_STATUS_X (48U)
#define TASK_STATUS_Y (26U)
#define TASK_STATUS_SIZE (16U)

typedef enum
{
    TASK_TERMINAL_NONE = 0,
    TASK_TERMINAL_FINISHED,
    TASK_TERMINAL_FAILED
} task_terminal_event_t;

extern volatile uint32_t sys_tick_ms;

static volatile task_id_t g_selected_task = TASK_ID_1;
static volatile task_state_t g_task_state = TASK_STATE_SELECTING;
static uint32_t g_task_start_ms = 0U;
static volatile uint32_t g_task_terminal_ms = 0U;
static volatile task_terminal_event_t g_task_terminal_pending =
    TASK_TERMINAL_NONE;
static uint32_t g_blink_reference_ms = 0U;
static uint32_t g_last_displayed_tenths = 0U;
static uint8_t g_selection_marker_visible = 1U;

static void task_manager_draw_task(void)
{
    uint8_t task_text[] = "TASK1";

    task_text[4] = (uint8_t)('0' + (uint8_t)g_selected_task);
    OLED_ShowString(TASK_TEXT_X, TASK_TEXT_Y, task_text, TASK_TEXT_SIZE);
}

static void task_manager_draw_selection_markers(uint8_t visible)
{
    uint8_t *left_marker = (uint8_t *)" ";
    uint8_t *right_marker = (uint8_t *)" ";

    if (visible != 0U)
    {
        left_marker = (uint8_t *)">";
        right_marker = (uint8_t *)"<";
    }

    OLED_ShowString(
        TASK_MARKER_LEFT_X, TASK_TEXT_Y, left_marker, TASK_TEXT_SIZE);
    OLED_ShowString(
        TASK_MARKER_RIGHT_X, TASK_TEXT_Y, right_marker, TASK_TEXT_SIZE);
}

static void task_manager_clear_time(void)
{
    OLED_ShowString(
        TASK_TIME_X, TASK_TIME_Y,
        (uint8_t *)"               ", TASK_TIME_SIZE);
}

static void task_manager_clear_status(void)
{
    OLED_ShowString(
        TASK_STATUS_X, TASK_STATUS_Y, (uint8_t *)"        ",
        TASK_STATUS_SIZE);
}

static void task_manager_draw_time(uint32_t elapsed_ms)
{
    char time_text[21];
    uint32_t elapsed_tenths = elapsed_ms / TASK_TIME_INTERVAL_MS;
    uint32_t whole_seconds = elapsed_tenths / 10U;
    uint32_t tenth_digit = elapsed_tenths % 10U;

    task_manager_clear_time();
    (void)snprintf(
        time_text, sizeof(time_text), "TIME:%lu.%lus",
        (unsigned long)whole_seconds, (unsigned long)tenth_digit);
    OLED_ShowString(
        TASK_TIME_X, TASK_TIME_Y, (uint8_t *)time_text, TASK_TIME_SIZE);
}

static void task_manager_draw_selection(void)
{
    task_manager_draw_task();
    task_manager_draw_selection_markers(g_selection_marker_visible);
    task_manager_clear_status();
    task_manager_clear_time();
    OLED_Refresh();
}

static void task_manager_draw_running(uint32_t elapsed_ms)
{
    task_manager_draw_task();
    task_manager_draw_selection_markers(0U);
    task_manager_clear_status();
    task_manager_draw_time(elapsed_ms);
    OLED_Refresh();
}

static void task_manager_draw_failed(uint32_t elapsed_ms)
{
    task_manager_draw_task();
    task_manager_draw_selection_markers(0U);
    task_manager_clear_status();
    OLED_ShowString(
        TASK_STATUS_X, TASK_STATUS_Y, (uint8_t *)"FAIL",
        TASK_STATUS_SIZE);
    task_manager_draw_time(elapsed_ms);
    OLED_Refresh();
}

static task_terminal_event_t task_manager_take_terminal_request(
    uint32_t *terminal_ms)
{
    uint32_t interrupt_state;
    task_terminal_event_t pending;

    interrupt_state = __get_PRIMASK();
    __disable_irq();
    pending = g_task_terminal_pending;
    if (pending != TASK_TERMINAL_NONE)
    {
        *terminal_ms = g_task_terminal_ms;
        g_task_terminal_pending = TASK_TERMINAL_NONE;
    }
    if (interrupt_state == 0U)
    {
        __enable_irq();
    }

    return pending;
}

static void task_manager_request_terminal(task_terminal_event_t event)
{
    uint32_t interrupt_state = __get_PRIMASK();

    __disable_irq();
    if ((g_task_state == TASK_STATE_RUNNING) &&
        (g_task_terminal_pending == TASK_TERMINAL_NONE))
    {
        g_task_terminal_ms = sys_tick_ms;
        g_task_terminal_pending = event;
    }
    if (interrupt_state == 0U)
    {
        __enable_irq();
    }
}

void task_manager_init(void)
{
    uint32_t now_ms = sys_tick_ms;

    g_selected_task = TASK_ID_1;
    g_task_state = TASK_STATE_SELECTING;
    g_task_start_ms = 0U;
    g_task_terminal_ms = 0U;
    g_task_terminal_pending = TASK_TERMINAL_NONE;
    g_blink_reference_ms = now_ms;
    g_last_displayed_tenths = 0U;
    g_selection_marker_visible = 1U;
    task_manager_draw_selection();
}

void task_manager_process(void)
{
    uint32_t now_ms = sys_tick_ms;
    uint32_t key_events = key_take_events();

    if (g_task_state == TASK_STATE_SELECTING)
    {
        if ((key_events & KEY_EVENT_CONFIRM) != 0U)
        {
            uint8_t complete_immediately;

            g_selection_marker_visible = 0U;
            g_last_displayed_tenths = 0U;
            /* 先初始化车辆和舵机，再开放 RUNNING 状态给两个控制中断。 */
            complete_immediately =
                competition_tasks_start(g_selected_task) ? 1U : 0U;
            g_task_start_ms = sys_tick_ms;
            g_task_state = TASK_STATE_RUNNING;
            if (complete_immediately != 0U)
            {
                task_manager_finish();
            }
            task_manager_draw_running(0U);
            return;
        }

        if ((key_events & KEY_EVENT_SELECT) != 0U)
        {
            if (g_selected_task >= TASK_ID_5)
            {
                g_selected_task = TASK_ID_1;
            }
            else
            {
                g_selected_task = (task_id_t)((uint32_t)g_selected_task + 1U);
            }
            g_selection_marker_visible = 1U;
            g_blink_reference_ms = now_ms;
            task_manager_draw_selection();
            return;
        }

        if ((uint32_t)(now_ms - g_blink_reference_ms) >=
            TASK_BLINK_INTERVAL_MS)
        {
            uint32_t elapsed_intervals =
                (uint32_t)(now_ms - g_blink_reference_ms) /
                TASK_BLINK_INTERVAL_MS;

            g_blink_reference_ms += elapsed_intervals * TASK_BLINK_INTERVAL_MS;
            if ((elapsed_intervals & 1U) != 0U)
            {
                g_selection_marker_visible =
                    (g_selection_marker_visible == 0U) ? 1U : 0U;
            }
            task_manager_draw_selection();
        }
        return;
    }

    if (g_task_state == TASK_STATE_RUNNING)
    {
        task_terminal_event_t terminal_event;
        uint32_t terminal_ms;
        uint32_t elapsed_ms;
        uint32_t elapsed_tenths;

        terminal_event =
            task_manager_take_terminal_request(&terminal_ms);
        if (terminal_event != TASK_TERMINAL_NONE)
        {
            elapsed_ms = (uint32_t)(terminal_ms - g_task_start_ms);
            g_last_displayed_tenths = elapsed_ms / TASK_TIME_INTERVAL_MS;
            if (terminal_event == TASK_TERMINAL_FAILED)
            {
                g_task_state = TASK_STATE_FAILED;
                task_manager_draw_failed(elapsed_ms);
            }
            else
            {
                g_task_state = TASK_STATE_FINISHED;
                task_manager_draw_running(elapsed_ms);
            }
            return;
        }

        elapsed_ms = (uint32_t)(now_ms - g_task_start_ms);
        elapsed_tenths = elapsed_ms / TASK_TIME_INTERVAL_MS;
        if (elapsed_tenths != g_last_displayed_tenths)
        {
            g_last_displayed_tenths = elapsed_tenths;
            task_manager_draw_running(elapsed_ms);
        }
    }
}

task_id_t task_manager_get_selected(void)
{
    return g_selected_task;
}

task_state_t task_manager_get_state(void)
{
    return g_task_state;
}

void task_manager_finish(void)
{
    task_manager_request_terminal(TASK_TERMINAL_FINISHED);
}

void task_manager_fail(void)
{
    task_manager_request_terminal(TASK_TERMINAL_FAILED);
}

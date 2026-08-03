#include "uart.h"

#include <limits.h>
#include <stddef.h>

#define UART_BALL_FRAME_BUFFER_SIZE (64U)
#define UART_BALL_FIELD_COUNT (7U)

extern volatile uint32_t sys_tick_ms;

volatile uart_ball_data_t g_uart_ball_data = {
    0U, -1, 0, 0, 0, 0, 0, 0U, 0U
};
volatile uint32_t g_uart_ball_parse_error_count = 0U;

static char g_uart_ball_frame_buffer[UART_BALL_FRAME_BUFFER_SIZE];
static uint8_t g_uart_ball_frame_index = 0U;
static bool g_uart_ball_discard_until_newline = false;

static bool UART_parse_int16(const char **cursor, int16_t *value)
{
    const char *position = *cursor;
    int32_t digit;
    int32_t magnitude = 0;
    int32_t limit = INT16_MAX;
    bool negative = false;

    if (*position == '-')
    {
        negative = true;
        limit = -(int32_t)INT16_MIN;
        position++;
    }

    if (*position < '0' || *position > '9')
    {
        return false;
    }

    while (*position >= '0' && *position <= '9')
    {
        digit = *position - '0';
        if (magnitude > (limit - digit) / 10)
        {
            return false;
        }
        magnitude = magnitude * 10 + digit;
        position++;
    }

    *value = negative ? (int16_t)(-magnitude) : (int16_t)magnitude;
    *cursor = position;
    return true;
}

static bool UART_parse_ball_frame(
    const char *frame, uart_ball_data_t *parsed)
{
    const char *cursor;
    int16_t fields[UART_BALL_FIELD_COUNT];
    uint8_t field_index;

    if (
        frame[0] != 'B' || frame[1] != 'A' ||
        frame[2] != 'L' || frame[3] != 'L' ||
        frame[4] != ','
    )
    {
        return false;
    }

    cursor = frame + 5;
    for (field_index = 0U;
         field_index < UART_BALL_FIELD_COUNT;
         field_index++)
    {
        if (!UART_parse_int16(&cursor, &fields[field_index]))
        {
            return false;
        }

        if (field_index + 1U < UART_BALL_FIELD_COUNT)
        {
            if (*cursor != ',')
            {
                return false;
            }
            cursor++;
        }
        else if (*cursor != '\0')
        {
            return false;
        }
    }

    if (fields[0] != 0 && fields[0] != 1)
    {
        return false;
    }

    parsed->valid = (uint8_t)fields[0];
    parsed->x = fields[1];
    parsed->error_neg11 = fields[2];
    parsed->error_neg5 = fields[3];
    parsed->error_zero = fields[4];
    parsed->error_pos5 = fields[5];
    parsed->error_pos11 = fields[6];
    return true;
}

static void UART_publish_ball_data(const uart_ball_data_t *parsed)
{
    uint32_t next_frame_count = g_uart_ball_data.frame_count + 1U;

    if (next_frame_count == 0U)
    {
        next_frame_count = 1U;
    }

    g_uart_ball_data.valid = parsed->valid;
    g_uart_ball_data.x = parsed->x;
    g_uart_ball_data.error_neg11 = parsed->error_neg11;
    g_uart_ball_data.error_neg5 = parsed->error_neg5;
    g_uart_ball_data.error_zero = parsed->error_zero;
    g_uart_ball_data.error_pos5 = parsed->error_pos5;
    g_uart_ball_data.error_pos11 = parsed->error_pos11;
    g_uart_ball_data.last_update_ms = sys_tick_ms;
    g_uart_ball_data.frame_count = next_frame_count;
}

static void UART_receive_ball_byte(uint8_t byte)
{
    uart_ball_data_t parsed;

    if (byte == '\r')
    {
        return;
    }

    if (byte == '\n')
    {
        if (g_uart_ball_discard_until_newline)
        {
            g_uart_ball_discard_until_newline = false;
            g_uart_ball_frame_index = 0U;
            return;
        }

        g_uart_ball_frame_buffer[g_uart_ball_frame_index] = '\0';
        if (UART_parse_ball_frame(g_uart_ball_frame_buffer, &parsed))
        {
            UART_publish_ball_data(&parsed);
        }
        else
        {
            g_uart_ball_parse_error_count++;
        }

        g_uart_ball_frame_index = 0U;
        return;
    }

    if (g_uart_ball_discard_until_newline)
    {
        return;
    }

    if (g_uart_ball_frame_index >= UART_BALL_FRAME_BUFFER_SIZE - 1U)
    {
        g_uart_ball_frame_index = 0U;
        g_uart_ball_discard_until_newline = true;
        g_uart_ball_parse_error_count++;
        return;
    }

    g_uart_ball_frame_buffer[g_uart_ball_frame_index] = (char)byte;
    g_uart_ball_frame_index++;
}

void UART_send_char(UART_Regs *uart, const uint8_t chr)
{
    DL_UART_transmitDataBlocking(uart, chr);
}

void UART_send_string(UART_Regs *uart, const char *str)
{
    while (*str)
    {
        UART_send_char(uart, (uint8_t)*str);
        str++;
    }
}

bool UART_get_ball_data(uart_ball_data_t *out)
{
    uint32_t frame_count_before;
    uint32_t frame_count_after;

    if (out == NULL)
    {
        return false;
    }

    do
    {
        frame_count_before = g_uart_ball_data.frame_count;
        out->valid = g_uart_ball_data.valid;
        out->x = g_uart_ball_data.x;
        out->error_neg11 = g_uart_ball_data.error_neg11;
        out->error_neg5 = g_uart_ball_data.error_neg5;
        out->error_zero = g_uart_ball_data.error_zero;
        out->error_pos5 = g_uart_ball_data.error_pos5;
        out->error_pos11 = g_uart_ball_data.error_pos11;
        out->last_update_ms = g_uart_ball_data.last_update_ms;
        frame_count_after = g_uart_ball_data.frame_count;
    } while (frame_count_before != frame_count_after);

    out->frame_count = frame_count_after;
    return frame_count_after != 0U;
}

void DEBUG_INST_IRQHandler(void)
{
    switch (DL_UART_getPendingInterrupt(DEBUG_INST))
    {
    case DL_UART_IIDX_RX:
    {
        uint8_t rec = DL_UART_receiveData(DEBUG_INST);
        UART_receive_ball_byte(rec);
        break;
    }

    default:
        break;
    }
}

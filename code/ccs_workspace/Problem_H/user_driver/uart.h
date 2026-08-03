#ifndef UART_H
#define UART_H

#include <stdbool.h>
#include <stdint.h>

#include "ti_msp_dl_config.h"

typedef struct
{
    uint8_t valid;
    int16_t x;
    int16_t error_neg11;
    int16_t error_neg5;
    int16_t error_zero;
    int16_t error_pos5;
    int16_t error_pos11;
    uint32_t frame_count;
    uint32_t last_update_ms;
} uart_ball_data_t;

extern volatile uart_ball_data_t g_uart_ball_data;
extern volatile uint32_t g_uart_ball_parse_error_count;

/**
 * @brief 串口发送字符
 * @param uart 端口
 * @param chr 字符
 */
void UART_send_char(UART_Regs *uart, const uint8_t chr);

/**
 * @brief 串口发送字符串
 * @param uart 端口
 * @param chr 字符串
 */
void UART_send_string(UART_Regs *uart, const char *str);

/**
 * @brief 获取最近一帧完整的 K230 钢球数据。
 * @param out 接收数据快照的指针。
 * @return true 表示至少收到过一帧合法数据，false 表示参数无效或尚未收到。
 */
bool UART_get_ball_data(uart_ball_data_t *out);

/**
 * @brief 接收串口信息中断函数
 */
void DEBUG_INST_IRQHandler(void);

#endif

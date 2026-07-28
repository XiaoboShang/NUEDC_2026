#ifndef UART_H
#define UART_H

#include "ti_msp_dl_config.h"

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
 * @brief 接收串口信息中断函数
 */
void DEBUG_INST_IRQHandler();

#endif
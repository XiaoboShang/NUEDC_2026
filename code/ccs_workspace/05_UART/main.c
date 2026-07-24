#include "ti_msp_dl_config.h"
#include "delay.h"
#include "oled.h"
#include "stdio.h"
#include "uart.h"

int main(void)
{
    SYSCFG_DL_init();
    OLED_Init(); // OLED初始化
    OLED_ColorTurn(0);
    OLED_DisplayTurn(0);
    OLED_Clear();
    NVIC_EnableIRQ(PRINT_INST_INT_IRQN); // 中断初始化

    while (1)
    {
        // char oled_str[50];
        // int int_a = 20;
        // sprintf(oled_str, "Integer : %d", int_a);
        // OLED_ShowString(0, 46, (u8 *)oled_str, 16);
        // OLED_Refresh();

        // OLED_ShowString(0, 0, (u8 *)"Hello World!", 16);
        // OLED_Refresh();

        delay_ms(500);
        DL_GPIO_clearPins(LED_PORT, LED_LED_0_PIN);
        delay_ms(500);
        DL_GPIO_setPins(LED_PORT, LED_LED_0_PIN);

        // 串口发送信息
        DL_UART_transmitData(PRINT_INST, 'H');
        UART_send_string(PRINT_INST, "Hello TI!\n");
    }
}

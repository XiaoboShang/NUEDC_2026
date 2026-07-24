#include "ti_msp_dl_config.h"
#include "delay.h"
#include "oled.h"
#include "stdio.h"
#include "uart.h"
#include "key.h"
#include "motor.h"

int status = 0;

int main(void)
{
    SYSCFG_DL_init();
    OLED_Init(); // OLED初始化
    OLED_ColorTurn(0);
    OLED_DisplayTurn(0);
    OLED_Clear();
    // NVIC_EnableIRQ(PRINT_INST_INT_IRQN); // 中断初始化
    NVIC_EnableIRQ(KEY_INT_IRQN);

    DL_Timer_startCounter(SERVO_INST);
    DL_Timer_setCaptureCompareValue(SERVO_INST, 50, GPIO_SERVO_C1_IDX); // 设置PWM波

    motor_init(1);
    motor_set_duty(1, 2000);

    while (1)
    {
        delay_ms(1000);
        motor_set_direction(1, 1);
        delay_ms(1000);
        motor_set_direction(1, 2);
        // switch (status)
        // {
        // case 0:
        //     OLED_Clear();
        //     OLED_ShowString(0, 0, (u8 *)"status : 0", 16);
        //     OLED_Refresh();
        //     break;
        // case 1:
        //     OLED_Clear();
        //     OLED_ShowString(0, 0, (u8 *)"status : 1", 16);
        //     OLED_Refresh();
        //     break;
        // case 2:
        //     OLED_Clear();
        //     OLED_ShowString(0, 0, (u8 *)"status : 2", 16);
        //     OLED_Refresh();
        //     break;

        // default:
        //     break;
        // }

        // char oled_str[50];
        // int int_a = 20;
        // sprintf(oled_str, "Integer : %d", int_a);
        // OLED_ShowString(0, 46, (u8 *)oled_str, 16);
        // OLED_Refresh();

        // OLED_ShowString(0, 0, (u8 *)"Hello World!", 16);
        // OLED_Refresh();

        // delay_ms(500);
        // DL_GPIO_clearPins(LED_PORT, LED_LED_0_PIN);
        // delay_ms(500);
        // DL_GPIO_setPins(LED_PORT, LED_LED_0_PIN);

        // // 串口发送信息
        // DL_UART_transmitData(PRINT_INST, 'H');
        // UART_send_string(PRINT_INST, "Hello TI!\n");
    }
}

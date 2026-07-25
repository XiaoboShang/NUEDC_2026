#include "ti_msp_dl_config.h"
#include "delay.h"
#include "oled.h"
#include "stdio.h"
#include "uart.h"
#include "key.h"
#include "motor.h"
#include "huidu.h"
int status = 0;
extern float target_speed_1;
extern float target_speed_2;
extern float speed_1;
extern float speed_2;
extern uint8_t huidu_value[];

int main(void)
{
    SYSCFG_DL_init();
    // OLED_Init(); // OLED初始化
    // OLED_ColorTurn(0);
    // OLED_DisplayTurn(0);
    // OLED_Clear();
    NVIC_EnableIRQ(GPIO_MULTIPLE_GPIOB_INT_IRQN);
    NVIC_EnableIRQ(DC_MOTOR_GPIOA_INT_IRQN);

    DL_Timer_startCounter(SERVO_INST);
    DL_Timer_setCaptureCompareValue(SERVO_INST, 50, GPIO_SERVO_C1_IDX); // 设置PWM波

    motor_init(1);
    motor_init(2);
    motor_set_direction(1, 1);
    motor_set_direction(2, 1);
    // motor_set_duty(1, 2000);
    target_speed_1 = 200; // mm/s
    target_speed_2 = 200; // mm/s

    // char huidu_buf[] = "00000\n";
    char current_speed[50];
    while (1)
    {
        sprintf(current_speed, "speed_1 : %.2f, speed_2 : %.2f\n", speed_1, speed_2);
        UART_send_string(DEBUG_INST, current_speed);

        // huidu_get_value();
        // sprintf(huidu_buf, "%d%d%d%d%d\n", huidu_value[0], huidu_value[1], huidu_value[2], huidu_value[3], huidu_value[4]);
        // UART_send_string(DEBUG_INST, huidu_buf);

        delay_ms(500);

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
    }
}

#include "ti_msp_dl_config.h"
#include "delay.h"
#include "oled.h"
#include "stdio.h"
#include "uart.h"
#include "key.h"
#include "motor.h"
#include "huidu.h"
#include "pid.h"
int status = 0;

int main(void)
{
    SYSCFG_DL_init();
    // OLED_Init(); // OLED初始化
    // OLED_ColorTurn(0);
    // OLED_DisplayTurn(0);
    // OLED_Clear();
    NVIC_EnableIRQ(DC_MOTOR_GPIOA_INT_IRQN);
    NVIC_EnableIRQ(DC_MOTOR_GPIOB_INT_IRQN);

    DL_Timer_startCounter(SERVO_INST);
    DL_Timer_setCaptureCompareValue(SERVO_INST, 50, GPIO_SERVO_C1_IDX); // 设置PWM波

    motor_init(1);
    motor_init(2);
    motor_set_direction(1, 1);
    motor_set_direction(2, 1);
    // motor_set_duty(1, 2000);
    target_speed_1 = 0; // mm/s
    target_speed_2 = 0; // mm/s
    line_pid_init();

    char huidu_buf[128];
    char current_speed[50];
    uint8_t huidu_snapshot[8];
    float line_error_snapshot;
    line_state_t line_state_snapshot;
    uint8_t index;
    while (1)
    {
        sprintf(current_speed, "speed_1 : %.2f, speed_2 : %.2f\n", speed_1, speed_2);
        UART_send_string(DEBUG_INST, current_speed);

        for (index = 0U; index < 8U; index++)
        {
            huidu_snapshot[index] = huidu_value[index];
        }
        line_error_snapshot = line_pid_get_error();
        line_state_snapshot = line_pid_get_state();
        sprintf(huidu_buf,
                "L4:%d,L3:%d,L2:%d,L1:%d,R1:%d,R2:%d,R3:%d,R4:%d,error:%.2f,state:%d\n",
                huidu_snapshot[0], huidu_snapshot[1], huidu_snapshot[2], huidu_snapshot[3],
                huidu_snapshot[4], huidu_snapshot[5], huidu_snapshot[6], huidu_snapshot[7],
                line_error_snapshot, (int)line_state_snapshot);
        UART_send_string(DEBUG_INST, huidu_buf);

        delay_ms(500);

        // OLED_ShowString(0, 0, (u8 *)"Hello World!", 16);
        // OLED_Refresh();

        // delay_ms(500);
        // DL_GPIO_clearPins(LED_PORT, LED_LED_0_PIN);
        // delay_ms(500);
        // DL_GPIO_setPins(LED_PORT, LED_LED_0_PIN);
    }
}

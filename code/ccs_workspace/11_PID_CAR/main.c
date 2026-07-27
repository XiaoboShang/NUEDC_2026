#include "ti_msp_dl_config.h"
#include "delay.h"
#include "oled.h"
#include "stdio.h"
#include "uart.h"
#include "key.h"
#include "motor.h"
#include "huidu.h"
#include "pid.h"
#include "mpu_port.h"
extern volatile uint32_t sys_tick_ms;
void SysTick_Handler(void)
{
    sys_tick_ms++;
}

int status = 0;

int main(void)
{
    SYSCFG_DL_init();
    OLED_Init(); // OLED初始化
    OLED_ColorTurn(0);
    OLED_DisplayTurn(0);
    OLED_Clear();
    DMP_Init();

    NVIC_EnableIRQ(DC_MOTOR_GPIOA_INT_IRQN);
    NVIC_EnableIRQ(DC_MOTOR_GPIOB_INT_IRQN);

    DL_Timer_startCounter(SERVO_INST);
    DL_Timer_setCaptureCompareValue(SERVO_INST, 50, GPIO_SERVO_C1_IDX); // 设置PWM波

    motor_init(1);
    motor_init(2);
    motor_set_direction(1, 1);
    motor_set_direction(2, 1);
    target_speed_1 = 10; // mm/s
    target_speed_2 = 10; // mm/s
    line_pid_init();

    // char huidu_buf[128];
    // char current_speed[50];
    // uint8_t huidu_snapshot[8];
    // float line_error_snapshot;
    // line_state_t line_state_snapshot;
    // uint8_t index;
    float pitch = 0, roll = 0, yaw = 0;
    char angle_pry[50];
    while (1)
    {
        // sprintf(current_speed, "speed_1 : %.2f, speed_2 : %.2f\n", speed_1, speed_2);
        // UART_send_string(DEBUG_INST, current_speed);

        // for (index = 0U; index < 8U; index++)
        // {
        //     huidu_snapshot[index] = huidu_value[index];
        // }
        // line_error_snapshot = line_pid_get_error();
        // line_state_snapshot = line_pid_get_state();
        // sprintf(huidu_buf,
        //         "L4:%d,L3:%d,L2:%d,L1:%d,R1:%d,R2:%d,R3:%d,R4:%d,error:%.2f,state:%d\n",
        //         huidu_snapshot[0], huidu_snapshot[1], huidu_snapshot[2], huidu_snapshot[3],
        //         huidu_snapshot[4], huidu_snapshot[5], huidu_snapshot[6], huidu_snapshot[7],
        //         line_error_snapshot, (int)line_state_snapshot);
        // UART_send_string(DEBUG_INST, huidu_buf);

        DMP_Read_Data(&pitch, &roll, &yaw);
        snprintf(angle_pry, sizeof(angle_pry), "P: %7.2f      ", (double)pitch);
        OLED_ShowString(0, 0, (u8 *)angle_pry, 16);
        snprintf(angle_pry, sizeof(angle_pry), "R: %7.2f      ", (double)roll);
        OLED_ShowString(0, 16, (u8 *)angle_pry, 16);
        snprintf(angle_pry, sizeof(angle_pry), "Y: %7.2f      ", (double)yaw);
        OLED_ShowString(0, 32, (u8 *)angle_pry, 16);
        OLED_Refresh();

        delay_ms(50);
    }
}

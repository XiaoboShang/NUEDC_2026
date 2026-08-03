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
#include "duoji.h"
#include "task_manager.h"
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
    task_manager_init();
    while (DMP_Init())
        ;
    NVIC_EnableIRQ(DEBUG_INST_INT_IRQN);
    NVIC_EnableIRQ(DC_MOTOR_GPIOA_INT_IRQN);
    NVIC_EnableIRQ(GPIO_MULTIPLE_GPIOB_INT_IRQN);

    motor_init(1);
    motor_init(2);
    motor_set_direction(1, 1);
    motor_set_direction(2, 1);
    line_pid_init();
    duoji_init();
    delay_ms(1000U);

    NVIC_ClearPendingIRQ(BALL_CTRL_INST_INT_IRQN);
    NVIC_SetPriority(DEBUG_INST_INT_IRQN, 0U);
    NVIC_SetPriority(BALL_CTRL_INST_INT_IRQN, 1U);
    NVIC_EnableIRQ(BALL_CTRL_INST_INT_IRQN);

    DL_Timer_startCounter(BALL_CTRL_INST);
    while (1)
    {
        task_manager_process();
        delay_ms(50);
    }
}

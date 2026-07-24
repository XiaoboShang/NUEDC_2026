#include "motor.h"

void motor_init(uint8_t motor_id)
{
    DL_GPIO_setPins(DC_MOTOR_STBY_PORT, DC_MOTOR_STBY_PIN);
    if (motor_id == 1)
    {
        DL_Timer_startCounter(PWMA_INST);
        DL_GPIO_setPins(DC_MOTOR_AIN1_PORT, DC_MOTOR_AIN1_PIN);
        DL_GPIO_setPins(DC_MOTOR_AIN2_PORT, DC_MOTOR_AIN2_PIN);
        DL_Timer_setCaptureCompareValue(PWMA_INST, 0, GPIO_PWMA_C0_IDX);
    }
    else if (motor_id == 2)
    {
        //
        //
    }
    DL_Timer_startCounter(MOTOR_PID_INST);
    NVIC_EnableIRQ(MOTOR_PID_INST_INT_IRQN);
}

void motor_set_duty(uint8_t motor_id, uint32_t duty)
{
    if (duty > 4000)
    {
        duty = 4000;
    }
    if (motor_id == 1)
    {
        DL_Timer_setCaptureCompareValue(PWMA_INST, duty, GPIO_PWMA_C0_IDX);
    }
    else if (motor_id == 2)
    {
        //
    }
}

void motor_set_direction(uint8_t motor_id, uint8_t direction)
{
    if (motor_id == 1)
    {
        if (direction == 0)
        {
            DL_GPIO_setPins(DC_MOTOR_AIN1_PORT, DC_MOTOR_AIN1_PIN);
            DL_GPIO_setPins(DC_MOTOR_AIN2_PORT, DC_MOTOR_AIN2_PIN);
        }
        else if (direction == 1)
        {
            DL_GPIO_setPins(DC_MOTOR_AIN1_PORT, DC_MOTOR_AIN1_PIN);
            DL_GPIO_clearPins(DC_MOTOR_AIN2_PORT, DC_MOTOR_AIN2_PIN);
        }
        else if (direction == 2)
        {
            DL_GPIO_clearPins(DC_MOTOR_AIN1_PORT, DC_MOTOR_AIN1_PIN);
            DL_GPIO_setPins(DC_MOTOR_AIN2_PORT, DC_MOTOR_AIN2_PIN);
        }
    }
    else if (motor_id == 2)
    {
        //
    }
}

extern uint32_t counter_1_A;
float speed_1 = 0;
float speed_2 = 0;
void calculate_speed(uint8_t motor_id)
{
    if (motor_id == 1)
    {
        speed_1 = (float)counter_1_A / MOTOR_ENCODER * PI * MOTOR_WHEEL_D * 20; // 轮速 mm/s
        counter_1_A = 0;
    }
    else if (motor_id == 2)
    {
        //
    }
}
// 每隔50ms触发一次速度更新
void MOTOR_PID_INST_IRQHandler()
{
    switch (DL_Timer_getPendingInterrupt(MOTOR_PID_INST))
    {
    case DL_TIMER_IIDX_LOAD:
        calculate_speed(1);
        break;

    default:
        break;
    }
}
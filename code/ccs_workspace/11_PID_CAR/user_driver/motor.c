#include "motor.h"

void motor_init(uint8_t motor_id)
{
    DL_GPIO_setPins(DC_MOTOR_STBY_PORT, DC_MOTOR_STBY_PIN);
    if (motor_id == 1)
    {
        DL_Timer_startCounter(PWMAB_INST);
        DL_GPIO_setPins(DC_MOTOR_AIN1_PORT, DC_MOTOR_AIN1_PIN);
        DL_GPIO_setPins(DC_MOTOR_AIN2_PORT, DC_MOTOR_AIN2_PIN);
        DL_Timer_setCaptureCompareValue(PWMAB_INST, 0, GPIO_PWMAB_C0_IDX);
    }
    else if (motor_id == 2)
    {
        DL_Timer_startCounter(PWMAB_INST);
        DL_GPIO_setPins(DC_MOTOR_BIN1_PORT, DC_MOTOR_BIN1_PIN);
        DL_GPIO_setPins(DC_MOTOR_BIN2_PORT, DC_MOTOR_BIN2_PIN);
        DL_Timer_setCaptureCompareValue(PWMAB_INST, 0, GPIO_PWMAB_C1_IDX);
    }
    DL_Timer_startCounter(MOTOR_PID_INST);
    NVIC_EnableIRQ(MOTOR_PID_INST_INT_IRQN);
}

uint32_t limit_duty(uint32_t duty)
{
    if (duty > 4000)
    {
        duty = 4000;
    }
    if (duty <= 0)
    {
        duty = 0;
    }
    return duty;
}

void motor_set_duty(uint8_t motor_id, uint32_t duty)
{
    limit_duty(duty);
    if (motor_id == 1)
    {
        DL_Timer_setCaptureCompareValue(PWMAB_INST, duty, GPIO_PWMAB_C0_IDX);
    }
    else if (motor_id == 2)
    {
        DL_Timer_setCaptureCompareValue(PWMAB_INST, duty, GPIO_PWMAB_C1_IDX);
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
        if (direction == 0)
        {
            DL_GPIO_setPins(DC_MOTOR_BIN1_PORT, DC_MOTOR_BIN1_PIN);
            DL_GPIO_setPins(DC_MOTOR_BIN2_PORT, DC_MOTOR_BIN2_PIN);
        }
        else if (direction == 1)
        {
            DL_GPIO_setPins(DC_MOTOR_BIN1_PORT, DC_MOTOR_BIN1_PIN);
            DL_GPIO_clearPins(DC_MOTOR_BIN2_PORT, DC_MOTOR_BIN2_PIN);
        }
        else if (direction == 2)
        {
            DL_GPIO_clearPins(DC_MOTOR_BIN1_PORT, DC_MOTOR_BIN1_PIN);
            DL_GPIO_setPins(DC_MOTOR_BIN2_PORT, DC_MOTOR_BIN2_PIN);
        }
    }
}

extern uint32_t counter_1_A;
extern uint32_t counter_2_A;

float speed_1 = 0;
float speed_2 = 0;
void calculate_speed(uint8_t motor_id)
{
    if (motor_id == 1)
    {
        speed_1 = (float)counter_1_A / MOTOR_ENCODER * PI * MOTOR_WHEEL_D * 100; // 轮速 mm/s
        counter_1_A = 0;
    }
    else if (motor_id == 2)
    {
        speed_2 = (float)counter_2_A / MOTOR_ENCODER * PI * MOTOR_WHEEL_D * 100; // 轮速 mm/s
        counter_2_A = 0;
    }
}

// PID公式
float kp = 0.5;
float ki = 0.4;
float kd = 0;

uint16_t PWM_1_duty = 0;
float target_speed_1 = 0; // mm/s
float last_error_1 = 0;
float current_error_1 = 0;

uint16_t PWM_2_duty = 0;
float target_speed_2 = 0; // mm/s
float last_error_2 = 0;
float current_error_2 = 0;
void DC_MOTOR_PID(uint8_t motor_id)
{
    float error;
    if (motor_id == 1)
    {
        error = target_speed_1 - speed_1;
        current_error_1 = error;
        PWM_1_duty += (uint16_t)(kp * (current_error_1 - last_error_1) + ki * (current_error_1));
        PWM_1_duty = limit_duty(PWM_1_duty);
        last_error_1 = current_error_1;
        motor_set_duty(motor_id, PWM_1_duty);
    }
    if (motor_id == 2)
    {
        error = target_speed_2 - speed_2;
        current_error_2 = error;
        PWM_2_duty += (uint16_t)(kp * (current_error_2 - last_error_2) + ki * (current_error_2));
        PWM_2_duty = limit_duty(PWM_2_duty);
        last_error_2 = current_error_2;
        motor_set_duty(motor_id, PWM_2_duty);
    }
}

// 每隔10ms触发一次速度更新
void MOTOR_PID_INST_IRQHandler()
{
    switch (DL_Timer_getPendingInterrupt(MOTOR_PID_INST))
    {
    case DL_TIMER_IIDX_LOAD:
        // adjust_motor();
        calculate_speed(1);
        DC_MOTOR_PID(1);
        calculate_speed(2);
        DC_MOTOR_PID(2);
        break;

    default:
        break;
    }
}
#include "motor.h"
#include "pid.h"

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
    duty = limit_duty(duty);
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

#define SPEED_FILTER_SIZE 5

float speed_1 = 0;
float speed_2 = 0;

static uint32_t speed_count_1[SPEED_FILTER_SIZE] = {0};
static uint32_t speed_count_2[SPEED_FILTER_SIZE] = {0};
static uint32_t speed_count_sum_1 = 0;
static uint32_t speed_count_sum_2 = 0;
static uint8_t speed_count_index_1 = 0;
static uint8_t speed_count_index_2 = 0;

void calculate_speed(uint8_t motor_id)
{
    uint32_t current_count;

    if (motor_id == 1)
    {
        current_count = counter_1_A;
        counter_1_A = 0;

        speed_count_sum_1 -= speed_count_1[speed_count_index_1];
        speed_count_1[speed_count_index_1] = current_count;
        speed_count_sum_1 += current_count;
        speed_count_index_1 = (speed_count_index_1 + 1) % SPEED_FILTER_SIZE;

        speed_1 = (float)speed_count_sum_1 / SPEED_FILTER_SIZE / MOTOR_ENCODER * PI * MOTOR_WHEEL_D * 100; // 轮速 mm/s
    }
    else if (motor_id == 2)
    {
        current_count = counter_2_A;
        counter_2_A = 0;

        speed_count_sum_2 -= speed_count_2[speed_count_index_2];
        speed_count_2[speed_count_index_2] = current_count;
        speed_count_sum_2 += current_count;
        speed_count_index_2 = (speed_count_index_2 + 1) % SPEED_FILTER_SIZE;

        speed_2 = (float)speed_count_sum_2 / SPEED_FILTER_SIZE / MOTOR_ENCODER * PI * MOTOR_WHEEL_D * 100; // 轮速 mm/s
    }
}

// PID公式
float kp = 0.5;
float ki = 0.4;
float kd = 0;

float PWM_1_duty = 0;
float target_speed_1 = 0; // mm/s
float last_error_1 = 0;
float current_error_1 = 0;

float PWM_2_duty = 0;
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
        PWM_1_duty += kp * (current_error_1 - last_error_1) + ki * current_error_1;
        if (PWM_1_duty > 4000)
        {
            PWM_1_duty = 4000;
        }
        if (PWM_1_duty < 0)
        {
            PWM_1_duty = 0;
        }
        last_error_1 = current_error_1;
        motor_set_duty(motor_id, (uint32_t)PWM_1_duty);
    }
    if (motor_id == 2)
    {
        error = target_speed_2 - speed_2;
        current_error_2 = error;
        PWM_2_duty += kp * (current_error_2 - last_error_2) + ki * current_error_2;
        if (PWM_2_duty > 4000)
        {
            PWM_2_duty = 4000;
        }
        if (PWM_2_duty < 0)
        {
            PWM_2_duty = 0;
        }
        last_error_2 = current_error_2;
        motor_set_duty(motor_id, (uint32_t)PWM_2_duty);
    }
}

// 每隔10ms触发一次速度更新
void MOTOR_PID_INST_IRQHandler()
{
    switch (DL_Timer_getPendingInterrupt(MOTOR_PID_INST))
    {
    case DL_TIMER_IIDX_LOAD:
        line_pid_update();
        calculate_speed(1);
        calculate_speed(2);
        DC_MOTOR_PID(1);
        DC_MOTOR_PID(2);
        judge_ads();

        break;

    default:
        break;
    }
}

#ifndef MOTOR_H
#define MOTOR_H

#define PI 3.14

// 编码器线数
#define MOTOR_ENCODER 390

// 轮胎直径 mm
#define MOTOR_WHEEL_D 65

// 3507 TB6612
// PB24 STBY
// PA8 AIN1
// PA9 AIN2
// PA12 PWMA
// GND GND
// 3v3 Vcc

// TB6612 电源模块
// vm 7.4v
// GND GND

// TB6612 直流电机1
// AO1 M+
// AO2 M-

// G3507 直流电机1
// PA21 A
// PA22 B

#include "ti_msp_dl_config.h"

extern float speed_1;
extern float speed_2;
extern float target_speed_1;
extern float target_speed_2;
/**
 * @brief 初始化电机
 * @param motor_id 电机编号可选1,2
 */
void motor_init(uint8_t motor_id);

/**
 * @brief 设置pwm占空比
 * @param motor_id 电机编号可选1,2
 * @param duty 占空比参数0-4000
 */
void motor_set_duty(uint8_t motor_id, uint32_t duty);

/**
 * @brief 设置电机转向
 * @param motor_id 电机编号可选1 2
 * @param direction 0不动 1正转 2反转
 */
void motor_set_direction(uint8_t motor_id, uint8_t direction);

/** Update both encoder speed feedback values. Call from the 10 ms motor ISR. */
void motor_update_speed_feedback(void);

/** Run both wheel speed PID controllers. Call from the 10 ms motor ISR. */
void motor_update_speed_pid(void);

/** Clear the accumulated encoder travel distance for both wheels. */
void motor_reset_odometry(void);

/** Return the average travel distance of both wheels in millimetres. */
float motor_get_average_distance_mm(void);

/** Stop both wheels and clear speed targets, PWM accumulation and PID history. */
void motor_stop_all(void);

#endif

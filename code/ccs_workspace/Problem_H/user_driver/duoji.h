#ifndef DUOJI_H
#define DUOJI_H

#include <stdbool.h>
#include <stdint.h>

#include "uart.h"

typedef enum
{
    DUOJI_CTRL_OK = 0,
    DUOJI_CTRL_NO_UART_DATA,
    DUOJI_CTRL_UART_TIMEOUT,
    DUOJI_CTRL_DUPLICATE_FRAME,
    DUOJI_CTRL_BALL_INVALID,
    DUOJI_CTRL_CALIBRATION_INVALID
} duoji_ball_control_status_t;

/** 钢球闭环算法的任务级可调参数。 */
typedef struct
{
    float velocity_filter_alpha;              /* 速度低通滤波系数，范围 0～1 */
    float proportional_gain_deg_per_cm;       /* 位置比例增益，单位 °/cm */
    float derivative_gain_deg_per_cms;        /* 速度微分增益，单位 °/(cm/s) */
    float approach_brake_gain;                 /* 接近目标时的附加微分增益 */
    float approach_brake_distance_cm;          /* 接近刹车作用距离，单位 cm */
    float gravity_bias_deg_per_cm;             /* 重力补偿偏置，单位 °/cm */
    float slow_integral_gain;                  /* 慢积分增益，单位 °/(cm·s) */
    float slow_integral_limit_deg;             /* 慢积分输出限幅，单位 ° */
    float slow_integral_active_distance_cm;    /* 慢积分启用距离，单位 cm */
    float target_ramp_rate_cm_s;               /* 目标渐变速率，单位 cm/s */
    float position_deadband_cm;                /* 目标位置死区，单位 cm */
    float max_tilt_deg;                        /* 相对水平角的最大倾斜，单位 ° */
    float max_servo_rate_deg_s;                /* 舵机最大变化速率，单位 °/s */
    float acceleration_feedforward_tilt_deg;   /* 满加速时的前馈倾斜角，单位 ° */
    float deceleration_feedforward_tilt_deg;   /* 满减速时的前馈倾斜角，单位 ° */
} duoji_ball_control_config_t;

/*
 * Servo PWM configuration used by this project:
 * timer clock = 1 MHz, timer resolution = 1 us,
 * PWM period = 20 ms (50 Hz).
 *
 * Start with the conservative 1.0 ms to 2.0 ms pulse range. If the actual
 * servo supports a wider range, calibrate these two values gradually.
 */
#define DUOJI_MIN_PULSE_US (1000U)
#define DUOJI_CENTER_PULSE_US (1500U)
#define DUOJI_MAX_PULSE_US (2000U)
#define DUOJI_MAX_ANGLE_DEG (180U)

/* 钢球闭环控制的公共机械标定和安全范围。 */
#define DUOJI_BALL_LEVEL_ANGLE_DEG (145.0f)
#define DUOJI_BALL_MIN_ANGLE_DEG (110.0f)
#define DUOJI_BALL_MAX_ANGLE_DEG (180.0f)
#define DUOJI_BALL_CONTROL_DIRECTION (1.0f)
#define DUOJI_BALL_UART_TIMEOUT_MS (200U)
#define DUOJI_BALL_DEFAULT_DT_S (0.05f)
#define DUOJI_BALL_MIN_DT_S (0.01f)
#define DUOJI_BALL_MAX_DT_S (0.20f)

#define DUOJI_BALL_MIN_POSITION_CM (-11.0f)
#define DUOJI_BALL_MAX_POSITION_CM (11.0f)
#define DUOJI_BALL_DEFAULT_TARGET_CM (0.0f)

/**
 * @brief Initialize the servo PWM output and move the pipe to level.
 * @note SYSCFG_DL_init() must be called before this function.
 */
void duoji_init(void);

/**
 * @brief Set the target servo angle.
 * @param angle_deg Target angle in degrees. Values above 180 are clamped.
 */
void duoji_set_angle(uint16_t angle_deg);

/**
 * @brief Set the servo high-level pulse width directly.
 * @param pulse_us Pulse width in microseconds. The value is clamped to the
 *                 range DUOJI_MIN_PULSE_US to DUOJI_MAX_PULSE_US.
 */
void duoji_set_pulse_us(uint16_t pulse_us);

/**
 * @brief 加载当前任务的钢球闭环参数，并清除上一任务的控制历史。
 * @param config 参数配置地址；传入空指针时保持当前配置不变。
 * @note 应在任务进入 RUNNING 前调用。
 */
void duoji_ball_control_set_config(
    const duoji_ball_control_config_t *config);

/**
 * @brief 设置车辆命令加速度比例，供钢球舵机前馈补偿使用。
 * @param ratio +1 表示满加速，0 表示匀速或静止，-1 表示满减速。
 * @note 输入会在舵机模块内部限制到 -1.0～1.0。
 */
void duoji_ball_control_set_vehicle_acceleration_ratio(float ratio);

/**
 * @brief 设置叠加在闭环输出之外的持久倾角，单位 °。
 * @param tilt_deg 负值减小舵机角，正值增大舵机角；输入受机械角范围限制。
 * @note 该倾角不会被 duoji_ball_control_reset() 清除，应在切换任务时显式清零。
 */
void duoji_ball_control_set_persistent_tilt_deg(float tilt_deg);

/**
 * @brief 清除闭环历史并立即输出“水平角 + 持久倾角”。
 * @note 用于尚未运行闭环、但需要保持持久倾角的阶段。
 */
void duoji_ball_control_hold_persistent_tilt(void);

/** Set and clamp the target ball position to -11 cm through +11 cm. */
void duoji_ball_control_set_target_cm(float target_cm);

/**
 * Reset ball-control history and immediately return the pipe to
 * "level + persistent tilt". A zero persistent tilt still returns to level.
 */
void duoji_ball_control_reset(void);

/**
 * @brief 开关重力补偿分支；开启后同时启用目标渐变、接近刹车、
 *        重力偏置和慢积分，具体强度由当前任务配置决定。
 */
void duoji_ball_set_gravity_compensation(bool enable);

/**
 * @brief Convert one UART ball-data snapshot to the calibrated position.
 * @param ball_data Complete UART snapshot containing the five calibration errors.
 * @param position_cm Receives the position clamped to -11 cm through +11 cm.
 * @return true when the pointers and calibration points are valid.
 * @note This function only performs conversion and does not change servo state.
 */
bool duoji_ball_data_to_position_cm(
    const uart_ball_data_t *ball_data, float *position_cm);

/**
 * Run one 10 ms control tick using the latest UART ball data.
 * @return true only when a new valid visual measurement was processed.
 *         Duplicate frames can still advance timed pulse and servo states.
 */
bool duoji_ball_control_update(void);

duoji_ball_control_status_t duoji_ball_control_get_status(void);
float duoji_ball_control_get_target_cm(void);
float duoji_ball_control_get_position_cm(void);
/** Return the low-pass-filtered ball velocity in cm/s. */
float duoji_ball_control_get_velocity_cm_s(void);
float duoji_ball_control_get_angle_deg(void);

#endif

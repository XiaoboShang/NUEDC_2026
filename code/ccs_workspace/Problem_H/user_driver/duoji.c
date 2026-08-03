#include "duoji.h"

#include "competition_tasks.h"
#include "ti_msp_dl_config.h"
#include "uart.h"

#define DUOJI_TIMER_TICK_US (1U)

extern volatile uint32_t sys_tick_ms;

/* 上电默认值与 Task2/Task3 的初始配置一致，任务启动后会加载各自配置。 */
static duoji_ball_control_config_t g_duoji_ball_control_config = {
    0.80f,  /* velocity_filter_alpha */
    2.30f,  /* proportional_gain_deg_per_cm */
    0.50f,  /* derivative_gain_deg_per_cms */
    1.20f,  /* approach_brake_gain */
    8.00f,  /* approach_brake_distance_cm */
    1.20f,  /* gravity_bias_deg_per_cm */
    2.00f,  /* slow_integral_gain */
    6.00f,  /* slow_integral_limit_deg */
    3.00f,  /* slow_integral_active_distance_cm */
    20.0f,  /* target_ramp_rate_cm_s */
    0.10f,  /* position_deadband_cm */
    12.0f,  /* max_tilt_deg */
    300.0f, /* max_servo_rate_deg_s */
    0.0f,   /* acceleration_feedforward_tilt_deg */
    0.0f    /* deceleration_feedforward_tilt_deg */
};

static float g_duoji_ball_target_cm = DUOJI_BALL_DEFAULT_TARGET_CM;
static float g_duoji_ball_ramp_target_cm = DUOJI_BALL_DEFAULT_TARGET_CM;
static float g_duoji_ball_position_cm = 0.0f;
static float g_duoji_ball_command_angle_deg =
    DUOJI_BALL_LEVEL_ANGLE_DEG;
static float g_duoji_ball_persistent_tilt_deg = 0.0f;
static float g_duoji_ball_filtered_velocity_cm_s = 0.0f;
static float g_duoji_ball_previous_position_cm = 0.0f;
static uint32_t g_duoji_ball_last_frame_count = 0U;
static uint32_t g_duoji_ball_previous_update_ms = 0U;
static uint32_t g_duoji_ball_previous_servo_update_ms = 0U;
static bool g_duoji_ball_measurement_initialized = false;
static bool g_duoji_ball_servo_timing_initialized = false;
static bool g_duoji_gravity_compensation_enabled = false;
static float g_duoji_slow_integral = 0.0f;
static volatile float g_duoji_vehicle_acceleration_ratio = 0.0f;
static duoji_ball_control_status_t g_duoji_ball_control_status =
    DUOJI_CTRL_NO_UART_DATA;

static float duoji_limit_float(float value, float minimum, float maximum)
{
    if (value < minimum)
    {
        return minimum;
    }
    if (value > maximum)
    {
        return maximum;
    }
    return value;
}

static float duoji_abs_float(float value)
{
    return value < 0.0f ? -value : value;
}

static void duoji_set_angle_float(float angle_deg)
{
    float pulse_us;

    angle_deg = duoji_limit_float(
        angle_deg, 0.0f, (float)DUOJI_MAX_ANGLE_DEG);
    pulse_us = (float)DUOJI_MIN_PULSE_US +
               angle_deg *
                   (float)(DUOJI_MAX_PULSE_US - DUOJI_MIN_PULSE_US) /
                   (float)DUOJI_MAX_ANGLE_DEG;

    duoji_set_pulse_us((uint16_t)(pulse_us + 0.5f));
}

static void duoji_ball_reset_control_history(void)
{
    g_duoji_ball_filtered_velocity_cm_s = 0.0f;
    g_duoji_ball_previous_position_cm = 0.0f;
    g_duoji_ball_previous_update_ms = 0U;
    g_duoji_ball_previous_servo_update_ms = 0U;
    g_duoji_ball_measurement_initialized = false;
    g_duoji_ball_servo_timing_initialized = false;
}

static void duoji_ball_return_to_persistent_tilt(void)
{
    duoji_ball_reset_control_history();
    g_duoji_ball_command_angle_deg = duoji_limit_float(
        DUOJI_BALL_LEVEL_ANGLE_DEG +
            g_duoji_ball_persistent_tilt_deg,
        DUOJI_BALL_MIN_ANGLE_DEG,
        DUOJI_BALL_MAX_ANGLE_DEG);
    duoji_set_angle_float(g_duoji_ball_command_angle_deg);
}

static float duoji_interpolate_position(
    int32_t ball_x, int32_t x0, int32_t x1,
    float position0, float position1)
{
    return position0 +
           ((float)(ball_x - x0) * (position1 - position0)) /
               (float)(x1 - x0);
}

bool duoji_ball_data_to_position_cm(
    const uart_ball_data_t *ball_data, float *position_cm)
{
    int32_t ball_x;
    int32_t x_neg11;
    int32_t x_neg5;
    int32_t x_zero;
    int32_t x_pos5;
    int32_t x_pos11;

    if ((ball_data == 0) || (position_cm == 0) ||
        (ball_data->valid == 0U))
    {
        return false;
    }

    ball_x = (int32_t)ball_data->x;
    x_neg11 = ball_x - (int32_t)ball_data->error_neg11;
    x_neg5 = ball_x - (int32_t)ball_data->error_neg5;
    x_zero = ball_x - (int32_t)ball_data->error_zero;
    x_pos5 = ball_x - (int32_t)ball_data->error_pos5;
    x_pos11 = ball_x - (int32_t)ball_data->error_pos11;

    if (
        x_neg11 >= x_neg5 || x_neg5 >= x_zero ||
        x_zero >= x_pos5 || x_pos5 >= x_pos11)
    {
        return false;
    }

    if (ball_x <= x_neg11)
    {
        *position_cm = DUOJI_BALL_MIN_POSITION_CM;
    }
    else if (ball_x < x_neg5)
    {
        *position_cm = duoji_interpolate_position(
            ball_x, x_neg11, x_neg5, -11.0f, -5.0f);
    }
    else if (ball_x < x_zero)
    {
        *position_cm = duoji_interpolate_position(
            ball_x, x_neg5, x_zero, -5.0f, 0.0f);
    }
    else if (ball_x < x_pos5)
    {
        *position_cm = duoji_interpolate_position(
            ball_x, x_zero, x_pos5, 0.0f, 5.0f);
    }
    else if (ball_x < x_pos11)
    {
        *position_cm = duoji_interpolate_position(
            ball_x, x_pos5, x_pos11, 5.0f, 11.0f);
    }
    else
    {
        *position_cm = DUOJI_BALL_MAX_POSITION_CM;
    }

    return true;
}

static uint32_t duoji_pulse_us_to_compare(uint16_t pulse_us)
{
    return (uint32_t)pulse_us / DUOJI_TIMER_TICK_US;
}

void duoji_init(void)
{
    g_duoji_ball_target_cm = DUOJI_BALL_DEFAULT_TARGET_CM;
    g_duoji_ball_position_cm = 0.0f;
    g_duoji_ball_last_frame_count = 0U;
    g_duoji_ball_persistent_tilt_deg = 0.0f;
    duoji_ball_return_to_persistent_tilt();
    DL_Timer_startCounter(DUOJI_INST);
}

void duoji_set_angle(uint16_t angle_deg)
{
    uint32_t pulse_us;

    if (angle_deg > DUOJI_MAX_ANGLE_DEG)
    {
        angle_deg = DUOJI_MAX_ANGLE_DEG;
    }

    pulse_us = DUOJI_MIN_PULSE_US +
               ((uint32_t)angle_deg *
                (DUOJI_MAX_PULSE_US - DUOJI_MIN_PULSE_US)) /
                   DUOJI_MAX_ANGLE_DEG;

    duoji_set_pulse_us((uint16_t)pulse_us);
}

void duoji_set_pulse_us(uint16_t pulse_us)
{
    if (pulse_us < DUOJI_MIN_PULSE_US)
    {
        pulse_us = DUOJI_MIN_PULSE_US;
    }
    else if (pulse_us > DUOJI_MAX_PULSE_US)
    {
        pulse_us = DUOJI_MAX_PULSE_US;
    }

    DL_Timer_setCaptureCompareValue(
        DUOJI_INST,
        duoji_pulse_us_to_compare(pulse_us),
        GPIO_DUOJI_C1_IDX);
}

void duoji_ball_control_set_config(
    const duoji_ball_control_config_t *config)
{
    if (config == 0)
    {
        return;
    }

    g_duoji_ball_control_config = *config;
    duoji_ball_control_reset();
}

void duoji_ball_control_set_vehicle_acceleration_ratio(float ratio)
{
    g_duoji_vehicle_acceleration_ratio =
        duoji_limit_float(ratio, -1.0f, 1.0f);
}

void duoji_ball_control_set_persistent_tilt_deg(float tilt_deg)
{
    g_duoji_ball_persistent_tilt_deg = duoji_limit_float(
        tilt_deg,
        DUOJI_BALL_MIN_ANGLE_DEG - DUOJI_BALL_LEVEL_ANGLE_DEG,
        DUOJI_BALL_MAX_ANGLE_DEG - DUOJI_BALL_LEVEL_ANGLE_DEG);
}

void duoji_ball_control_hold_persistent_tilt(void)
{
    duoji_ball_return_to_persistent_tilt();
}

void duoji_ball_control_set_target_cm(float target_cm)
{
    float limited_target = duoji_limit_float(
        target_cm,
        DUOJI_BALL_MIN_POSITION_CM,
        DUOJI_BALL_MAX_POSITION_CM);

    if (limited_target != g_duoji_ball_target_cm)
    {
        g_duoji_ball_target_cm = limited_target;
        /* ramp 从球当前位置开始，不是从旧 ramp 值开始 */
        g_duoji_ball_ramp_target_cm = g_duoji_ball_position_cm;
        g_duoji_slow_integral = 0.0f;
    }
}

void duoji_ball_control_reset(void)
{
    g_duoji_ball_last_frame_count = 0U;
    g_duoji_ball_ramp_target_cm = g_duoji_ball_target_cm;
    g_duoji_slow_integral = 0.0f;
    g_duoji_vehicle_acceleration_ratio = 0.0f;
    g_duoji_ball_control_status = DUOJI_CTRL_NO_UART_DATA;
    duoji_ball_return_to_persistent_tilt();
}

void duoji_ball_set_gravity_compensation(bool enable)
{
    g_duoji_gravity_compensation_enabled = enable;
    if (!enable)
    {
        g_duoji_slow_integral = 0.0f;
    }
}

bool duoji_ball_control_update(void)
{
    uart_ball_data_t ball_data;
    uint32_t now_ms;
    uint32_t measurement_delta_ms;
    uint32_t servo_delta_ms;
    float measurement_dt_s;
    float servo_dt_s;
    float raw_velocity_cm_s;
    float position_error_cm;
    float absolute_position_error_cm;
    float desired_angle_deg;
    float limited_angle_deg;
    float angle_delta;
    float maximum_angle_delta;
    bool new_measurement;

    if (!UART_get_ball_data(&ball_data))
    {
        g_duoji_ball_control_status = DUOJI_CTRL_NO_UART_DATA;
        duoji_ball_return_to_persistent_tilt();
        return false;
    }

    now_ms = sys_tick_ms;
    if ((uint32_t)(now_ms - ball_data.last_update_ms) >
        DUOJI_BALL_UART_TIMEOUT_MS)
    {
        g_duoji_ball_control_status = DUOJI_CTRL_UART_TIMEOUT;
        duoji_ball_return_to_persistent_tilt();
        return false;
    }

    new_measurement =
        ball_data.frame_count != g_duoji_ball_last_frame_count;
    if (new_measurement)
    {
        g_duoji_ball_last_frame_count = ball_data.frame_count;
    }

    if (!ball_data.valid)
    {
        g_duoji_ball_control_status = DUOJI_CTRL_BALL_INVALID;
        duoji_ball_return_to_persistent_tilt();
        return false;
    }

    if (new_measurement &&
        !duoji_ball_data_to_position_cm(
            &ball_data, &g_duoji_ball_position_cm))
    {
        g_duoji_ball_control_status = DUOJI_CTRL_CALIBRATION_INVALID;
        duoji_ball_return_to_persistent_tilt();
        return false;
    }

    if (new_measurement)
    {
        if (g_duoji_ball_measurement_initialized)
        {
            measurement_delta_ms = (uint32_t)(
                ball_data.last_update_ms -
                g_duoji_ball_previous_update_ms);
            measurement_dt_s = duoji_limit_float(
                (float)measurement_delta_ms / 1000.0f,
                DUOJI_BALL_MIN_DT_S,
                DUOJI_BALL_MAX_DT_S);
            raw_velocity_cm_s =
                (g_duoji_ball_position_cm -
                 g_duoji_ball_previous_position_cm) /
                measurement_dt_s;
            g_duoji_ball_filtered_velocity_cm_s +=
                g_duoji_ball_control_config.velocity_filter_alpha *
                (raw_velocity_cm_s -
                 g_duoji_ball_filtered_velocity_cm_s);
        }
        else
        {
            g_duoji_ball_filtered_velocity_cm_s = 0.0f;
            g_duoji_ball_measurement_initialized = true;
        }

        g_duoji_ball_previous_position_cm =
            g_duoji_ball_position_cm;
        g_duoji_ball_previous_update_ms =
            ball_data.last_update_ms;
    }
    else if (!g_duoji_ball_measurement_initialized)
    {
        g_duoji_ball_control_status = DUOJI_CTRL_DUPLICATE_FRAME;
        return false;
    }

    /* 舵机时序必须在 ramp 和 PD 之前计算 */
    if (g_duoji_ball_servo_timing_initialized)
    {
        servo_delta_ms = (uint32_t)(
            now_ms - g_duoji_ball_previous_servo_update_ms);
        servo_dt_s = duoji_limit_float(
            (float)servo_delta_ms / 1000.0f,
            DUOJI_BALL_MIN_DT_S,
            DUOJI_BALL_MAX_DT_S);
    }
    else
    {
        servo_dt_s = DUOJI_BALL_DEFAULT_DT_S;
        g_duoji_ball_servo_timing_initialized = true;
    }
    g_duoji_ball_previous_servo_update_ms = now_ms;

    /*
     * 目标渐变仅在重力补偿开启时生效，防止目标跳变造成过冲；
     * 关闭重力补偿时直接使用最终目标。
     */
    if (g_duoji_gravity_compensation_enabled)
    {
        float ramp_step =
            g_duoji_ball_control_config.target_ramp_rate_cm_s *
            servo_dt_s;
        float ramp_error =
            g_duoji_ball_target_cm - g_duoji_ball_ramp_target_cm;

        if (duoji_abs_float(ramp_error) <= ramp_step)
        {
            g_duoji_ball_ramp_target_cm = g_duoji_ball_target_cm;
        }
        else if (ramp_error > 0.0f)
        {
            g_duoji_ball_ramp_target_cm += ramp_step;
        }
        else
        {
            g_duoji_ball_ramp_target_cm -= ramp_step;
        }
    }
    else
    {
        /* 关闭重力补偿时直接跳到目标。 */
        g_duoji_ball_ramp_target_cm = g_duoji_ball_target_cm;
    }

    /* 误差用渐变后的 ramp 目标计算，而非最终目标 */
    position_error_cm =
        g_duoji_ball_ramp_target_cm - g_duoji_ball_position_cm;
    absolute_position_error_cm = duoji_abs_float(position_error_cm);

    /*
     * 距离比例 PD 控制 + 重力补偿：
     *   tilt = bias + P * error  -  D * velocity
     *
     *   bias：管子中间低两边高，目标偏离中心越远，
     *         舵机需要越大的稳态倾斜来抵抗重力。
     *         目标 -5cm → bias = -5 * 0.6 = -3°
     *
     *   P * error：球还没到目标时额外加力
     *   D * velocity：球速越快越刹车，防过冲
     */
    {
        float gravity_bias;
        float proportional_tilt;
        float derivative_tilt;
        float feedforward_tilt;
        float vehicle_acceleration_ratio;
        float total_tilt;

        /* 重力补偿：仅在使能时生效，用最终目标位置算（稳态维持力）。
         * 注意：必须用最终目标而非 ramp，否则切换方向时 ramp 还在
         * 旧位置，bias 会短暂指向反方向，造成"先推反再推正"的双推。 */
        gravity_bias = 0.0f;
        if (g_duoji_gravity_compensation_enabled)
        {
            gravity_bias =
                g_duoji_ball_control_config.gravity_bias_deg_per_cm *
                g_duoji_ball_target_cm *
                DUOJI_BALL_CONTROL_DIRECTION;
        }

        proportional_tilt =
            g_duoji_ball_control_config.proportional_gain_deg_per_cm *
            position_error_cm *
            DUOJI_BALL_CONTROL_DIRECTION;

        /* 基础微分阻尼 */
        derivative_tilt =
            g_duoji_ball_control_config.derivative_gain_deg_per_cms *
            g_duoji_ball_filtered_velocity_cm_s *
            DUOJI_BALL_CONTROL_DIRECTION;

        /*
         * 接近刹车仅在重力补偿开启时生效。
         * 球距目标近时始终抑制速度，不区分方向。
         * 去掉 error*velocity>0 条件，否则球冲过目标后刹车
         * 立即关闭，球自由滑行到更远位置才停下。
         */
        if (g_duoji_gravity_compensation_enabled &&
            absolute_position_error_cm <
                g_duoji_ball_control_config.approach_brake_distance_cm)
        {
            derivative_tilt +=
                g_duoji_ball_control_config.approach_brake_gain *
                g_duoji_ball_filtered_velocity_cm_s *
                DUOJI_BALL_CONTROL_DIRECTION;
        }

        total_tilt = gravity_bias + proportional_tilt -
                     derivative_tilt;

        /*
         * 车辆加减速前馈：正比例使用加速角，负比例使用减速角。
         * 该项只叠加倾斜量，后续仍统一经过最大倾角、机械角和速率限制。
         */
        vehicle_acceleration_ratio = duoji_limit_float(
            g_duoji_vehicle_acceleration_ratio, -1.0f, 1.0f);
        if (vehicle_acceleration_ratio > 0.0f)
        {
            feedforward_tilt =
                g_duoji_ball_control_config
                    .acceleration_feedforward_tilt_deg *
                vehicle_acceleration_ratio;
        }
        else
        {
            feedforward_tilt =
                g_duoji_ball_control_config
                    .deceleration_feedforward_tilt_deg *
                (-vehicle_acceleration_ratio);
        }

        /*
         * 慢积分：重力补偿开启时持续累积，逐步修正稳态偏差。
         * 球卡在 -3cm 到不了 -5cm？积分一点点加力，直到推到位。
         * 只在误差较小时累积，避免大误差时积分饱和。
         */
        if (g_duoji_gravity_compensation_enabled &&
            absolute_position_error_cm <
                g_duoji_ball_control_config.slow_integral_active_distance_cm)
        {
            g_duoji_slow_integral +=
                g_duoji_ball_control_config.slow_integral_gain *
                position_error_cm * servo_dt_s;
            g_duoji_slow_integral = duoji_limit_float(
                g_duoji_slow_integral,
                -g_duoji_ball_control_config.slow_integral_limit_deg,
                g_duoji_ball_control_config.slow_integral_limit_deg);

            if (absolute_position_error_cm <=
                g_duoji_ball_control_config.position_deadband_cm)
            {
                /* 死区内：只用 bias + 积分，PD 停掉防微振 */
                total_tilt = gravity_bias + g_duoji_slow_integral;
            }
            else
            {
                /* 死区外：PD + bias + 积分一起上 */
                total_tilt += g_duoji_slow_integral;
            }
        }

        total_tilt += feedforward_tilt;

        total_tilt = duoji_limit_float(
            total_tilt,
            -g_duoji_ball_control_config.max_tilt_deg,
            g_duoji_ball_control_config.max_tilt_deg);

        /*
         * 持久倾角在原闭环的最大倾角限幅之后叠加，避免 Task5
         * 弯管补偿被旧的 ±max_tilt_deg 限幅吞掉。
         */
        desired_angle_deg = DUOJI_BALL_LEVEL_ANGLE_DEG + total_tilt +
                            g_duoji_ball_persistent_tilt_deg;
    }

    limited_angle_deg = duoji_limit_float(
        desired_angle_deg,
        DUOJI_BALL_MIN_ANGLE_DEG,
        DUOJI_BALL_MAX_ANGLE_DEG);

    angle_delta =
        limited_angle_deg - g_duoji_ball_command_angle_deg;
    maximum_angle_delta =
        g_duoji_ball_control_config.max_servo_rate_deg_s *
        servo_dt_s;
    angle_delta = duoji_limit_float(
        angle_delta,
        -maximum_angle_delta,
        maximum_angle_delta);
    g_duoji_ball_command_angle_deg = duoji_limit_float(
        g_duoji_ball_command_angle_deg + angle_delta,
        DUOJI_BALL_MIN_ANGLE_DEG,
        DUOJI_BALL_MAX_ANGLE_DEG);

    duoji_set_angle_float(g_duoji_ball_command_angle_deg);

    if (new_measurement)
    {
        g_duoji_ball_control_status = DUOJI_CTRL_OK;
        return true;
    }

    g_duoji_ball_control_status = DUOJI_CTRL_DUPLICATE_FRAME;
    return false;
}

duoji_ball_control_status_t duoji_ball_control_get_status(void)
{
    return g_duoji_ball_control_status;
}

float duoji_ball_control_get_target_cm(void)
{
    return g_duoji_ball_target_cm;
}

float duoji_ball_control_get_position_cm(void)
{
    return g_duoji_ball_position_cm;
}

float duoji_ball_control_get_velocity_cm_s(void)
{
    return g_duoji_ball_filtered_velocity_cm_s;
}

float duoji_ball_control_get_angle_deg(void)
{
    return g_duoji_ball_command_angle_deg;
}

void BALL_CTRL_INST_IRQHandler(void)
{
    switch (DL_Timer_getPendingInterrupt(BALL_CTRL_INST))
    {
    case DL_TIMER_IIDX_LOAD:
        /* 由任务执行层决定是否运行小球视觉闭环。 */
        competition_tasks_ball_isr_update();
        break;

    default:
        break;
    }
}

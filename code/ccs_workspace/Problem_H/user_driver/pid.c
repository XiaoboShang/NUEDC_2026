#include "pid.h"
#include "huidu.h"
#include "motor.h"

/* ======================== 巡线 PID 可调参数区 ======================== */

#define LINE_INTEGRAL_LIMIT 20.0f // 误差积分绝对值上限；减小可降低积分饱和风险

#define LINE_FINISH_BLACK_THRESHOLD 4U // 单次检测到至少 4 路黑色时锁定终点
#define LINE_FINISH_DRIVE_COUNT 40U    // 锁定后继续循迹 50 个 10 ms 周期，即 500 ms
#define LINE_LOST_TIMEOUT_COUNT 50U    // 丢线搜索超时次数；10 ms 周期下 50 次等于 500 ms
#define LINE_CONTROL_PERIOD_S 0.01f

/* 默认配置用于上电初始化；每个任务可在启动时加载自己的独立配置。 */
static line_pid_config_t line_pid_config = {
    18.0f,  /* kp */
    0.0f,   /* ki */
    35.0f,  /* kd */
    250.0f, /* base_speed */
    120.0f, /* min_base_speed */
    450.0f, /* max_target_speed */
    180.0f, /* max_correction */
    150.0f, /* search_speed */
    0.0f,   /* max_acceleration：0 表示不限制 */
    0.0f,   /* max_deceleration：0 表示不限制 */
    1U      /* finish_detection_enabled */
};

/* 八路传感器顺序：L4、L3、L2、L1、R1、R2、R3、R4。 */
static const float line_sensor_weight[8] = {
    -7.0f, -5.0f, -3.0f, -1.0f,
    1.0f, 3.0f, 5.0f, 7.0f};

/* ======================== 巡线 PID 运行状态区 ======================== */

static float line_error_sum = 0.0f;                          // 每 10 ms 累计一次的误差积分，仅在正常巡线时更新
static float line_last_error = 0.0f;                         // 上一次 PID 误差，用于计算微分项
static float line_last_valid_error = 0.0f;                   // 最近一次检测到黑线时的误差，用于决定丢线搜索方向
static volatile float line_current_error = 0.0f;             // 当前误差，由 10 ms 中断更新，主循环可读取
static uint16_t line_lost_count = 0U;                        // 连续全白周期数，每个计数代表 10 ms
static uint8_t line_finish_latched = 0U;                     // 已检测到终点标志后保持锁定
static uint8_t line_finish_drive_count = 0U;                 // 终点锁定后的继续循迹周期数
static volatile line_state_t line_state = LINE_STATE_NORMAL; // 当前状态，由 10 ms 中断更新
static uint8_t line_pid_initialized = 0U;                    // 初始化标志，防止定时器先启动时误输出目标速度
static volatile float line_requested_speed_scale = 1.0f;     // 任务层请求的速度比例
static float line_current_speed_scale = 0.0f;                // 经过加减速限制后的实际速度比例

/**
 * @brief 将浮点数限制在指定范围内。
 * @param value 待限制的数值。
 * @param minimum 允许的最小值。
 * @param maximum 允许的最大值。
 * @return 限幅后的数值。
 */
static float line_limit_float(float value, float minimum, float maximum)
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

/**
 * @brief 按当前任务的加减速度限制更新实际速度比例。
 * @param 无。
 * @return 无。
 */
static void line_update_speed_scale(void)
{
    float target_speed;
    float current_speed;
    float maximum_delta;

    if (line_pid_config.base_speed <= 0.0f)
    {
        line_current_speed_scale = 0.0f;
        return;
    }

    target_speed = line_pid_config.base_speed *
                   line_limit_float(
                       line_requested_speed_scale, 0.0f, 1.0f);
    current_speed = line_pid_config.base_speed *
                    line_current_speed_scale;

    if (target_speed > current_speed)
    {
        if (line_pid_config.max_acceleration <= 0.0f)
        {
            current_speed = target_speed;
        }
        else
        {
            maximum_delta = line_pid_config.max_acceleration *
                            LINE_CONTROL_PERIOD_S;
            if ((target_speed - current_speed) > maximum_delta)
            {
                current_speed += maximum_delta;
            }
            else
            {
                current_speed = target_speed;
            }
        }
    }
    else if (target_speed < current_speed)
    {
        if (line_pid_config.max_deceleration <= 0.0f)
        {
            current_speed = target_speed;
        }
        else
        {
            maximum_delta = line_pid_config.max_deceleration *
                            LINE_CONTROL_PERIOD_S;
            if ((current_speed - target_speed) > maximum_delta)
            {
                current_speed -= maximum_delta;
            }
            else
            {
                current_speed = target_speed;
            }
        }
    }

    line_current_speed_scale = line_limit_float(
        current_speed / line_pid_config.base_speed, 0.0f, 1.0f);
}

/**
 * @brief 统计当前八路灰度传感器中检测到黑色的数量。
 * @param 无。
 * @return 黑色传感器数量，范围为 0～8。
 */
static uint8_t line_get_black_count(void)
{
    uint8_t index;
    uint8_t black_count = 0U;

    for (index = 0U; index < 8U; index++)
    {
        if (huidu_value[index] != 0U)
        {
            black_count++;
        }
    }

    return black_count;
}

/**
 * @brief 计算所有黑色传感器权值的平均值。
 * @param black_count 当前检测到黑色的传感器数量，必须大于 0。
 * @return 黑线位置误差，范围为 -7.0～7.0；单位为传感器权值。
 */
static float line_calculate_error(uint8_t black_count)
{
    uint8_t index;
    float weight_sum = 0.0f;

    for (index = 0U; index < 8U; index++)
    {
        if (huidu_value[index] != 0U)
        {
            weight_sum += line_sensor_weight[index];
        }
    }

    return weight_sum / (float)black_count;
}

/**
 * @brief 根据当前位置误差计算左右轮差速修正量。
 * @param error 当前黑线位置误差，范围为 -7.0～7.0。
 * @return 差速修正量，单位 mm/s，范围由当前任务配置限制。
 */
static float line_pid_calculate(float error)
{
    float derivative;
    float correction;

    line_error_sum += error;
    line_error_sum = line_limit_float(line_error_sum,
                                      -LINE_INTEGRAL_LIMIT,
                                      LINE_INTEGRAL_LIMIT);

    derivative = error - line_last_error;
    correction = line_pid_config.kp * error +
                 line_pid_config.ki * line_error_sum +
                 line_pid_config.kd * derivative;
    line_last_error = error;

    return line_limit_float(correction,
                            -line_pid_config.max_correction,
                            line_pid_config.max_correction);
}

/**
 * @brief 根据偏差绝对值计算当前基准速度。
 * @param error 当前黑线位置误差，范围为 -7.0～7.0。
 * @return 基准速度，单位 mm/s，范围由当前任务配置决定。
 */
static float line_calculate_base_speed(float error)
{
    float absolute_error = error;
    float speed_range;
    float base_speed;

    if (absolute_error < 0.0f)
    {
        absolute_error = -absolute_error;
    }
    absolute_error = line_limit_float(absolute_error, 0.0f, 7.0f);

    speed_range = line_pid_config.base_speed -
                  line_pid_config.min_base_speed;
    base_speed = line_pid_config.base_speed -
                 speed_range * absolute_error / 7.0f;

    return line_limit_float(base_speed,
                            line_pid_config.min_base_speed,
                            line_pid_config.base_speed);
}

/**
 * @brief 限制并设置左右电机的目标速度。
 * @param left_speed 电机 1（左轮）目标速度，单位 mm/s。
 * @param right_speed 电机 2（右轮）目标速度，单位 mm/s。
 * @return 无。
 */
static void line_set_target_speed(float left_speed, float right_speed)
{
    target_speed_1 = line_limit_float(
        left_speed, 0.0f, line_pid_config.max_target_speed);
    target_speed_2 = line_limit_float(
        right_speed, 0.0f, line_pid_config.max_target_speed);
}

/**
 * @brief 处理八路全白的丢线状态，并按最后一次有效误差方向低速搜索。
 * @param 无。
 * @return 无。搜索超过 500 ms 后进入锁定停车状态。
 */
static void line_handle_lost(void)
{
    line_state = LINE_STATE_LOST_SEARCH;

    if (line_lost_count < LINE_LOST_TIMEOUT_COUNT)
    {
        line_lost_count++;
    }

    if (line_lost_count >= LINE_LOST_TIMEOUT_COUNT)
    {
        line_state = LINE_STATE_LOST_STOP;
        line_set_target_speed(0.0f, 0.0f);
        return;
    }

    if (line_last_valid_error > 0.0f)
    {
        line_set_target_speed(
            line_pid_config.search_speed * line_current_speed_scale,
            0.0f);
    }
    else if (line_last_valid_error < 0.0f)
    {
        line_set_target_speed(
            0.0f,
            line_pid_config.search_speed * line_current_speed_scale);
    }
    else
    {
        line_set_target_speed(
            line_pid_config.search_speed * line_current_speed_scale,
            line_pid_config.search_speed * line_current_speed_scale);
    }
}

/**
 * @brief 检测终点标志，并管理触发后的 500 ms 继续循迹计时。
 * @param black_count 当前检测到黑色的传感器数量。
 * @return 1 表示 500 ms 已到并进入停车状态，0 表示继续循迹。
 */
static uint8_t line_update_finish_delay(uint8_t black_count)
{
    if (line_pid_config.finish_detection_enabled == 0U)
    {
        return 0U;
    }

    if (line_finish_latched == 0U)
    {
        if (black_count < LINE_FINISH_BLACK_THRESHOLD)
        {
            return 0U;
        }

        /* 第一次检测到至少 4 路黑色时只锁定终点，本周期不计入 500 ms。 */
        line_finish_latched = 1U;
        line_finish_drive_count = 0U;
        line_lost_count = 0U;
        return 0U;
    }

    if (line_finish_drive_count < LINE_FINISH_DRIVE_COUNT)
    {
        line_finish_drive_count++;
    }

    if (line_finish_drive_count < LINE_FINISH_DRIVE_COUNT)
    {
        return 0U;
    }

    line_state = LINE_STATE_FINISH_STOP;
    line_error_sum = 0.0f;
    line_set_target_speed(0.0f, 0.0f);
    return 1U;
}

/**
 * @brief 初始化八路灰度巡线 PID 控制器。
 * @param 无。
 * @return 无。
 */
void line_pid_init(void)
{
    line_error_sum = 0.0f;
    line_last_error = 0.0f;
    line_last_valid_error = 0.0f;
    line_current_error = 0.0f;
    line_lost_count = 0U;
    line_finish_latched = 0U;
    line_finish_drive_count = 0U;
    line_state = LINE_STATE_NORMAL;
    line_requested_speed_scale = 1.0f;
    line_current_speed_scale = 0.0f;
    line_set_target_speed(0.0f, 0.0f);
    line_pid_initialized = 1U;
}

void line_pid_set_config(const line_pid_config_t *config)
{
    if (config == 0)
    {
        return;
    }

    /* 整体复制配置；调用方应保证此时巡线控制尚未进入 RUNNING。 */
    line_pid_config = *config;
}

void line_pid_set_speed_scale(float scale)
{
    line_requested_speed_scale = line_limit_float(scale, 0.0f, 1.0f);
}

float line_pid_get_current_speed_command_mm_s(void)
{
    return line_pid_config.base_speed * line_current_speed_scale;
}

bool line_pid_is_finish_marker_detected(void)
{
    return line_get_black_count() >= LINE_FINISH_BLACK_THRESHOLD;
}

/**
 * @brief 执行一次灰度采集、状态判断和巡线 PID 更新。
 * @param 无。
 * @return 无。
 * @note 该函数按 10 ms 固定周期调用，电机 1 为左轮，电机 2 为右轮。
 */
void line_pid_update(void)
{
    uint8_t black_count;
    uint8_t was_lost;
    float error;
    float correction;
    float base_speed;

    huidu_get_value();
    line_update_speed_scale();

    if (line_pid_initialized == 0U)
    {
        line_set_target_speed(0.0f, 0.0f);
        return;
    }

    if ((line_state == LINE_STATE_FINISH_STOP) || (line_state == LINE_STATE_LOST_STOP))
    {
        line_set_target_speed(0.0f, 0.0f);
        return;
    }

    black_count = line_get_black_count();

    /* 终点锁定后继续正常循迹，满 500 ms 才进入停车状态。 */
    if (line_update_finish_delay(black_count) != 0U)
    {
        return;
    }

    if (black_count == 0U)
    {
        line_handle_lost();
        return;
    }

    was_lost = (line_state == LINE_STATE_LOST_SEARCH) ? 1U : 0U;
    error = line_calculate_error(black_count);
    line_current_error = error;
    line_last_valid_error = error;
    line_lost_count = 0U;
    line_state = LINE_STATE_NORMAL;

    if (was_lost != 0U)
    {
        line_error_sum = 0.0f;
        line_last_error = error;
    }

    correction = line_pid_calculate(error) * line_current_speed_scale;
    base_speed = line_calculate_base_speed(error) *
                 line_current_speed_scale;
    line_set_target_speed(base_speed + correction,
                          base_speed - correction);
}

/**
 * @brief 清除终点或丢线超时产生的停车锁定并重新开始巡线。
 * @param 无。
 * @return 无。
 */
void line_pid_reset(void)
{
    line_pid_init();
}

/**
 * @brief 获取当前黑线的加权位置误差。
 * @param 无。
 * @return 当前误差，范围为 -7.0～7.0。
 */
float line_pid_get_error(void)
{
    return line_current_error;
}

/**
 * @brief 获取当前巡线控制器状态。
 * @param 无。
 * @return 当前巡线状态 line_state_t。
 */
line_state_t line_pid_get_state(void)
{
    return line_state;
}

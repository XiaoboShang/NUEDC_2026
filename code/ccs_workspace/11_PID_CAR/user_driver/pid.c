#include "pid.h"
#include "huidu.h"
#include "motor.h"

/* ======================== 巡线 PID 可调参数区 ======================== */

#define LINE_PID_KP                 35.0f  // 比例系数：建议 10～60；增大可加快转向，过大会左右振荡
#define LINE_PID_KI                  0.0f  // 积分系数：建议 0～5；增大可消除长期偏差，过大会产生积分饱和
#define LINE_PID_KD                 20.0f  // 微分系数：建议 0～40；增大可抑制摆动，过大会放大传感器跳变

#define LINE_BASE_SPEED            300.0f  // 直线巡航速度，单位 mm/s；增大可提速，但会降低弯道稳定性
#define LINE_MIN_BASE_SPEED        150.0f  // 大偏差时最低基准速度，单位 mm/s；减小可提高急弯通过能力
#define LINE_MAX_TARGET_SPEED      500.0f  // 单轮最高目标速度，单位 mm/s；应与电机速度环能力匹配
#define LINE_MAX_CORRECTION        300.0f  // 最大差速修正量，单位 mm/s；增大可增强急弯转向能力
#define LINE_INTEGRAL_LIMIT         20.0f  // 误差积分绝对值上限；减小可降低积分饱和风险

#define LINE_FINISH_CONFIRM_COUNT       3U // 连续全黑确认次数；10 ms 周期下 3 次等于 30 ms
#define LINE_LOST_TIMEOUT_COUNT        50U // 丢线搜索超时次数；10 ms 周期下 50 次等于 500 ms
#define LINE_SEARCH_SPEED           150.0f // 丢线搜索时外侧车轮速度，单位 mm/s；增大可加快搜索

/* 八路传感器顺序：L4、L3、L2、L1、R1、R2、R3、R4。 */
static const float line_sensor_weight[8] = {
    -7.0f, -5.0f, -3.0f, -1.0f,
     1.0f,  3.0f,  5.0f,  7.0f
};

/* ======================== 巡线 PID 运行状态区 ======================== */

static float line_error_sum = 0.0f;              // 每 10 ms 累计一次的误差积分，仅在正常巡线时更新
static float line_last_error = 0.0f;             // 上一次 PID 误差，用于计算微分项
static float line_last_valid_error = 0.0f;       // 最近一次检测到黑线时的误差，用于决定丢线搜索方向
static volatile float line_current_error = 0.0f; // 当前误差，由 10 ms 中断更新，主循环可读取
static uint16_t line_lost_count = 0U;            // 连续全白周期数，每个计数代表 10 ms
static uint8_t line_finish_count = 0U;           // 连续全黑周期数，每个计数代表 10 ms
static volatile line_state_t line_state = LINE_STATE_NORMAL; // 当前状态，由 10 ms 中断更新
static uint8_t line_pid_initialized = 0U;        // 初始化标志，防止定时器先启动时误输出目标速度

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
 * @return 差速修正量，单位 mm/s，范围由 LINE_MAX_CORRECTION 限制。
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
    correction = LINE_PID_KP * error
               + LINE_PID_KI * line_error_sum
               + LINE_PID_KD * derivative;
    line_last_error = error;

    return line_limit_float(correction,
                            -LINE_MAX_CORRECTION,
                            LINE_MAX_CORRECTION);
}

/**
 * @brief 根据偏差绝对值计算当前基准速度。
 * @param error 当前黑线位置误差，范围为 -7.0～7.0。
 * @return 基准速度，单位 mm/s，范围为 LINE_MIN_BASE_SPEED～LINE_BASE_SPEED。
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

    speed_range = LINE_BASE_SPEED - LINE_MIN_BASE_SPEED;
    base_speed = LINE_BASE_SPEED - speed_range * absolute_error / 7.0f;

    return line_limit_float(base_speed,
                            LINE_MIN_BASE_SPEED,
                            LINE_BASE_SPEED);
}

/**
 * @brief 限制并设置左右电机的目标速度。
 * @param left_speed 电机 1（左轮）目标速度，单位 mm/s。
 * @param right_speed 电机 2（右轮）目标速度，单位 mm/s。
 * @return 无。
 */
static void line_set_target_speed(float left_speed, float right_speed)
{
    target_speed_1 = line_limit_float(left_speed, 0.0f, LINE_MAX_TARGET_SPEED);
    target_speed_2 = line_limit_float(right_speed, 0.0f, LINE_MAX_TARGET_SPEED);
}

/**
 * @brief 处理八路全白的丢线状态，并按最后一次有效误差方向低速搜索。
 * @param 无。
 * @return 无。搜索超过 500 ms 后进入锁定停车状态。
 */
static void line_handle_lost(void)
{
    line_finish_count = 0U;
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
        line_set_target_speed(LINE_SEARCH_SPEED, 0.0f);
    }
    else if (line_last_valid_error < 0.0f)
    {
        line_set_target_speed(0.0f, LINE_SEARCH_SPEED);
    }
    else
    {
        line_set_target_speed(LINE_SEARCH_SPEED, LINE_SEARCH_SPEED);
    }
}

/**
 * @brief 处理八路全黑的终点确认状态。
 * @param 无。
 * @return 无。连续确认达到 30 ms 后进入锁定停车状态。
 */
static void line_handle_finish(void)
{
    line_lost_count = 0U;

    if (line_finish_count < LINE_FINISH_CONFIRM_COUNT)
    {
        line_finish_count++;
    }

    if (line_finish_count >= LINE_FINISH_CONFIRM_COUNT)
    {
        line_state = LINE_STATE_FINISH_STOP;
        line_error_sum = 0.0f;
        line_set_target_speed(0.0f, 0.0f);
    }
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
    line_finish_count = 0U;
    line_state = LINE_STATE_NORMAL;
    line_set_target_speed(0.0f, 0.0f);
    line_pid_initialized = 1U;
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

    if (line_pid_initialized == 0U)
    {
        line_set_target_speed(0.0f, 0.0f);
        return;
    }

    if ((line_state == LINE_STATE_FINISH_STOP)
        || (line_state == LINE_STATE_LOST_STOP))
    {
        line_set_target_speed(0.0f, 0.0f);
        return;
    }

    black_count = line_get_black_count();

    if (black_count == 0U)
    {
        line_handle_lost();
        return;
    }

    if (black_count == 8U)
    {
        line_handle_finish();
        return;
    }

    was_lost = (line_state == LINE_STATE_LOST_SEARCH) ? 1U : 0U;
    error = line_calculate_error(black_count);
    line_current_error = error;
    line_last_valid_error = error;
    line_lost_count = 0U;
    line_finish_count = 0U;
    line_state = LINE_STATE_NORMAL;

    if (was_lost != 0U)
    {
        line_error_sum = 0.0f;
        line_last_error = error;
    }

    correction = line_pid_calculate(error);
    base_speed = line_calculate_base_speed(error);
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

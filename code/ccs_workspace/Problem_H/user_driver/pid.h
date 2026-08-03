#ifndef PID_H
#define PID_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief 单个任务使用的巡线控制参数。
 * @note 速度单位为 mm/s，加速度和减速度单位为 mm/s^2。
 */
typedef struct
{
    float kp;
    float ki;
    float kd;
    float base_speed;
    float min_base_speed;
    float max_target_speed;
    float max_correction;
    float search_speed;
    float max_acceleration;
    float max_deceleration;
    uint8_t finish_detection_enabled;
} line_pid_config_t;

/**
 * @brief 巡线控制器的运行状态。
 */
typedef enum
{
    LINE_STATE_NORMAL = 0,
    LINE_STATE_LOST_SEARCH,
    LINE_STATE_FINISH_STOP,
    LINE_STATE_LOST_STOP
} line_state_t;

/**
 * @brief 初始化八路灰度巡线 PID 控制器。
 * @param 无。
 * @return 无。
 */
void line_pid_init(void);

/**
 * @brief 加载当前任务的巡线参数。
 * @param config 参数配置地址；传入空指针时保持原配置不变。
 * @note 应在任务进入 RUNNING 前调用，line_pid_reset() 不会覆盖该配置。
 */
void line_pid_set_config(const line_pid_config_t *config);

/**
 * @brief 设置巡线运行速度比例，范围为 0.0～1.0。
 * @param scale 0.0 表示平滑减速到停车，1.0 表示运行当前任务的完整速度。
 * @note 实际比例按照当前任务配置的加速度和减速度限制逐步变化。
 */
void line_pid_set_speed_scale(float scale);

/**
 * @brief 获取经过加减速斜坡处理后的当前基准速度命令。
 * @return 当前基准速度命令，单位 mm/s。
 */
float line_pid_get_current_speed_command_mm_s(void);

/**
 * @brief 判断最近一次灰度采样是否检测到横向启停线。
 * @return 至少 4 路传感器检测到黑色时返回 true，否则返回 false。
 * @note 该函数只读取 line_pid_update() 已采集的数据，不会再次访问 GPIO。
 */
bool line_pid_is_finish_marker_detected(void);

/**
 * @brief 执行一次灰度采集、状态判断和巡线 PID 更新。
 * @param 无。
 * @return 无。
 * @note 应由 10 ms 定时中断周期调用。
 */
void line_pid_update(void);

/**
 * @brief 清除终点或丢线超时产生的停车锁定并重新开始巡线。
 * @param 无。
 * @return 无。
 */
void line_pid_reset(void);

/**
 * @brief 获取当前黑线的加权位置误差。
 * @param 无。
 * @return 当前误差，范围为 -7.0～7.0；负值表示黑线在左侧，正值表示在右侧。
 */
float line_pid_get_error(void);

/**
 * @brief 获取当前巡线控制器状态。
 * @param 无。
 * @return 当前巡线状态 line_state_t。
 */
line_state_t line_pid_get_state(void);

#endif

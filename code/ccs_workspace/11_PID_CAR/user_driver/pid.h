#ifndef PID_H
#define PID_H

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

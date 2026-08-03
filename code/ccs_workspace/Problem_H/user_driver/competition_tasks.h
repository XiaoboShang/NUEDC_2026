#ifndef COMPETITION_TASKS_H
#define COMPETITION_TASKS_H

#include <stdbool.h>

#include "task_manager.h"

/**
 * @brief 在任务进入 RUNNING 前，初始化选中的竞赛任务。
 * @param task 当前选中的任务编号。
 * @return true 表示该任务初始化后应立即完成，false 表示继续运行。
 */
bool competition_tasks_start(task_id_t task);

/** @brief 由电机 10 ms 中断调用，分发当前任务的电机控制事件。 */
void competition_tasks_motor_isr_update(void);

/** @brief 由球控 10 ms 中断调用，分发当前任务的小球控制事件。 */
void competition_tasks_ball_isr_update(void);

#endif

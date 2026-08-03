#include "competition_tasks.h"

#include <stdbool.h>
#include <stdint.h>

#include "duoji.h"
#include "motor.h"
#include "pid.h"
#include "ti_msp_dl_config.h"
#include "uart.h"

/* Task2 的目标位置、允许误差和稳定时间。 */
#define TASK2_POSITIVE_TARGET_CM (5.0f)
#define TASK2_NEGATIVE_TARGET_CM (-5.0f)
#define TASK2_POSITIVE_PASS_THRESHOLD_CM (4.0f) /* 球越过此位置即认为到达 +5cm */
#define TASK2_NEGATIVE_TOLERANCE_CM (1.0f)      /* -5cm 处允许的 ± 误差 */
#define TASK2_VELOCITY_TOLERANCE_CM_S (0.5f)    /* 稳定判定的最大球速 */
#define TASK2_NEGATIVE_STABLE_MS (300U)         /* -5cm 稳定保持时间 */

/* Task3 的直线距离、速度曲线和故障判定参数。 */
#define TASK3_BALL_TARGET_CM (0.0f)
#define TASK3_APPROACH_B_DISTANCE_MM (1500.0f)
#define TASK3_B_DISTANCE_MM (1500.0f)
#define TASK3_BASE_SPEED_MM_S (230.0f) // task3基础速度
#define TASK3_CROSS_B_SPEED_MM_S (80.0f)
#define TASK3_STOP_SPEED_MM_S (20.0f)
#define TASK3_STOP_STABLE_MS (200U)
#define TASK3_POST_B_STOP_TIMEOUT_MS (2000U)
#define TASK3_MOTOR_CONTROL_PERIOD_S (0.01f)
#define TASK3_CROSS_B_SPEED_SCALE \
    (TASK3_CROSS_B_SPEED_MM_S / TASK3_BASE_SPEED_MM_S)

/* Task4 的整圈行驶、平滑停车和故障判定参数。 */
#define TASK4_BALL_TARGET_CM (-0.3f)
#define TASK4_BASE_SPEED_MM_S (230.0f)
#define TASK4_STOP_SPEED_MM_S (20.0f)
#define TASK4_STOP_STABLE_MS (200U)
#define TASK4_STOP_TIMEOUT_MS (5000U)
#define TASK4_MOTOR_CONTROL_PERIOD_S (0.01f)

/* Task5 的目标平均采样、整圈行驶和平滑停车参数。 */
#define TASK5_TARGET_AVERAGE_MS (3000U)
#define TASK5_CURVATURE_POSITIVE_MAX_DEG (20.0f)
#define TASK5_CURVATURE_NEGATIVE_MAX_DEG (10.0f)
#define TASK5_BASE_SPEED_MM_S (230.0f)
#define TASK5_STOP_SPEED_MM_S (20.0f)
#define TASK5_STOP_STABLE_MS (200U)
#define TASK5_STOP_TIMEOUT_MS (5000U)
#define TASK5_MOTOR_CONTROL_PERIOD_S (0.01f)

/*
 * Task2 专用舵机参数，修改会影响 +5 cm → -5 cm 测试。
 * 这里只调整 Task2；不会改变 Task3 的舵机参数。
 */
static const duoji_ball_control_config_t g_task2_ball_config = {
    0.80f,  /* 速度滤波系数：越大响应越快，但视觉噪声越明显，范围 0～1 */
    2.30f,  /* P增益，°/cm：越大到达目标越快，过大会来回振荡 */
    0.50f,  /* D增益，°/(cm/s)：越大刹车越强，过大会放大速度噪声 */
    1.20f,  /* 接近目标附加D增益：增大可减少过冲，过大会响应迟钝 */
    8.00f,  /* 接近刹车作用距离，cm：球进入该距离后启用附加阻尼 */
    1.20f,  /* 重力偏置，°/cm：补偿水管弧度，增大可提高稳态维持力 */
    2.00f,  /* 慢积分增益，°/(cm·s)：增大可消除静差，过大会低频摆动 */
    6.00f,  /* 慢积分最大输出，°：限制积分产生的最大额外倾斜 */
    3.00f,  /* 慢积分启用距离，cm：误差小于该值时才累计积分 */
    20.0f,  /* 目标渐变速度，cm/s：越小目标切换越平缓，但到达更慢 */
    0.10f,  /* 位置死区，cm：死区内停用PD，增大可减小抖动但降低精度 */
    12.0f,  /* 最大倾斜角，°：越大推球能力越强，过大容易冲过目标 */
    300.0f, /* 最大舵机速率，°/s：越小动作越平滑，但响应更慢 */
    0.0f,   /* 加速前馈角，°：Task2 车辆静止，保持关闭 */
    0.0f    /* 减速前馈角，°：Task2 车辆静止，保持关闭 */
};

/*
 * Task3 专用舵机参数，只修改此处不会影响 Task2。
 * 钢球目标为 0.5 cm，重力偏置会按该目标产生少量稳态倾斜。
 */
static const duoji_ball_control_config_t g_task3_ball_config = {
    0.80f,  /* 速度滤波系数：越大响应越快，但视觉噪声越明显，范围 0～1 */
    3.30f,  /* P增益，°/cm：越大回中心越快，过大会来回振荡 */
    0.60f,  /* D增益，°/(cm/s)：越大刹车越强，过大会放大速度噪声 */
    1.20f,  /* 接近中心附加D增益：增大可减少过冲，过大会响应迟钝 */
    8.00f,  /* 接近刹车作用距离，cm：球进入该距离后启用附加阻尼 */
    1.20f,  /* 重力偏置，°/cm：0.5cm目标时约产生0.6°稳态倾斜 */
    2.00f,  /* 慢积分增益，°/(cm·s)：增大可消除静差，过大会低频摆动 */
    6.00f,  /* 慢积分最大输出，°：限制积分产生的最大额外倾斜 */
    3.00f,  /* 慢积分启用距离，cm：误差小于该值时才累计积分 */
    20.0f,  /* 目标渐变速度，cm/s：Task3目标固定0.5cm，通常无需调整 */
    0.10f,  /* 位置死区，cm：死区内停用PD，增大可减小抖动但降低精度 */
    12.0f,  /* 最大倾斜角，°：越大抗车体扰动能力越强，过大容易过冲 */
    300.0f, /* 最大舵机速率，°/s：越小动作越平滑，但响应更慢 */
    10.0f,  // -2.1f,  /* 加速前馈角，°：负值减小舵机角，使水管向车尾倾斜；建议每次调 0.2° */
    -10.0f, // 3.0f    /* 减速前馈角，°：正值增大舵机角，使水管向车头倾斜；建议每次调 0.2° */
};

/* Task4 独立舵机参数；初值复制 Task3，后续调参互不影响。 */
static const duoji_ball_control_config_t g_task4_ball_config = {
    0.80f,  /* 速度滤波系数 */
    3.30f,  /* P增益，°/cm */
    0.60f,  /* D增益，°/(cm/s) */
    1.20f,  /* 接近中心附加D增益 */
    8.00f,  /* 接近刹车作用距离，cm */
    1.20f,  /* 重力偏置，°/cm */
    2.00f,  /* 慢积分增益，°/(cm·s) */
    6.00f,  /* 慢积分最大输出，° */
    3.00f,  /* 慢积分启用距离，cm */
    20.0f,  /* 目标渐变速度，cm/s */
    0.10f,  /* 位置死区，cm */
    12.0f,  /* 最大倾斜角，° */
    300.0f, /* 最大舵机速率，°/s */
    5.0f,   /* 加速前馈角，° */
    -5.0f   /* 减速前馈角，° */
};

/* Task5 独立舵机参数；初值复制 Task4，后续调参互不影响。 */
static const duoji_ball_control_config_t g_task5_ball_config = {
    0.80f,  /* 速度滤波系数 */
    3.30f,  /* P增益，°/cm */
    0.60f,  /* D增益，°/(cm/s) */
    1.20f,  /* 接近目标附加D增益 */
    8.00f,  /* 接近刹车作用距离，cm */
    1.20f,  /* 重力偏置，°/cm */
    2.00f,  /* 慢积分增益，°/(cm·s) */
    6.00f,  /* 慢积分最大输出，° */
    3.00f,  /* 慢积分启用距离，cm */
    20.0f,  /* 目标渐变速度，cm/s */
    0.10f,  /* 位置死区，cm */
    12.0f,  /* 最大倾斜角，° */
    300.0f, /* 最大舵机速率，°/s */
    5.0f,   /* 加速前馈角，° */
    -5.0f   /* 减速前馈角，° */
};

/*
 * Task1 独立使用的巡线参数。
 * 以后 Task3/Task4 需要巡线时，应分别定义自己的配置并在 START 事件中加载。
 */
static const line_pid_config_t g_task1_line_config = {
    18.0f,  /* kp */
    0.0f,   /* ki */
    35.0f,  /* kd */
    400.0f, /* base_speed */
    300.0f, /* min_base_speed */
    500.0f, /* max_target_speed */
    180.0f, /* max_correction */
    150.0f, /* search_speed */
    0.0f,   /* max_acceleration：Task1 保持原来的立即响应 */
    0.0f,   /* max_deceleration：Task1 保持原来的立即响应 */
    1U      /* finish_detection_enabled */
};

/* Task3 使用较低巡航速度和独立的平滑启停参数。 */
static const line_pid_config_t g_task3_line_config = {
    18.0f,                 /* kp */
    0.0f,                  /* ki */
    35.0f,                 /* kd */
    TASK3_BASE_SPEED_MM_S, /* base_speed */
    210.0f,                /* min_base_speed */
    250.0f,                /* max_target_speed */
    150.0f,                /* max_correction */
    120.0f,                /* search_speed */
    80.0f,                 /* max_acceleration，mm/s^2 */
    80.0f,                 /* max_deceleration，mm/s^2 */
    0U                     /* 禁用 A 点横向启停线检测 */
};

/* Task4 独立循迹参数；初值复制 Task3，由 Task4 状态机处理 A 点停车。 */
static const line_pid_config_t g_task4_line_config = {
    18.0f,                 /* kp */
    0.0f,                  /* ki */
    35.0f,                 /* kd */
    TASK4_BASE_SPEED_MM_S, /* base_speed */
    210.0f,                /* min_base_speed */
    250.0f,                /* max_target_speed */
    150.0f,                /* max_correction */
    120.0f,                /* search_speed */
    80.0f,                 /* max_acceleration，mm/s^2 */
    180.0f,                /* max_deceleration，mm/s^2 */
    0U                     /* 由 Task4 状态机直接处理四黑终点 */
};

/* Task5 独立循迹参数；初值复制 Task4。 */
static const line_pid_config_t g_task5_line_config = {
    18.0f,                 /* kp */
    0.0f,                  /* ki */
    35.0f,                 /* kd */
    TASK5_BASE_SPEED_MM_S, /* base_speed */
    210.0f,                /* min_base_speed */
    250.0f,                /* max_target_speed */
    150.0f,                /* max_correction */
    120.0f,                /* search_speed */
    80.0f,                 /* max_acceleration，mm/s^2 */
    180.0f,                /* max_deceleration，mm/s^2 */
    0U                     /* 由 Task5 状态机直接处理四黑终点 */
};

/*
 * 每个任务函数都会收到以下三种事件之一。
 * 这样既能把同一任务的逻辑集中在一个函数内，又不会混淆两个 10 ms 中断。
 */
typedef enum
{
    TASK_EVENT_START = 0,
    TASK_EVENT_MOTOR_10MS,
    TASK_EVENT_BALL_10MS
} competition_task_event_t;

/* Task2 的运行阶段。 */
typedef enum
{
    TASK2_PHASE_IDLE = 0,
    TASK2_PHASE_MOVE_POSITIVE,
    TASK2_PHASE_MOVE_NEGATIVE,
    TASK2_PHASE_HOLD_NEGATIVE
} task2_phase_t;

/* Task3 的运行阶段；该变量会在电机和球控两个中断之间共享。 */
typedef enum
{
    TASK3_PHASE_IDLE = 0,
    TASK3_PHASE_DRIVE,
    TASK3_PHASE_APPROACH_B,
    TASK3_PHASE_POST_B_STOP,
    TASK3_PHASE_HOLD_CENTER,
    TASK3_PHASE_FAILED
} task3_phase_t;

/* Task4 的运行阶段；该变量会在电机和球控两个中断之间共享。 */
typedef enum
{
    TASK4_PHASE_IDLE = 0,
    TASK4_PHASE_DRIVE,
    TASK4_PHASE_STOPPING,
    TASK4_PHASE_HOLD_CENTER,
    TASK4_PHASE_FAILED
} task4_phase_t;

/* Task5 的运行阶段；该变量会在电机和球控两个中断之间共享。 */
typedef enum
{
    TASK5_PHASE_IDLE = 0,
    TASK5_PHASE_AVERAGE_TARGET,
    TASK5_PHASE_DRIVE,
    TASK5_PHASE_STOPPING,
    TASK5_PHASE_HOLD_TARGET,
    TASK5_PHASE_FAILED
} task5_phase_t;

extern volatile uint32_t sys_tick_ms;

static volatile task2_phase_t g_task2_phase = TASK2_PHASE_IDLE;
static uint32_t g_task2_stable_start_ms = 0U;
static uint8_t g_task2_stable_active = 0U;
static volatile task3_phase_t g_task3_phase = TASK3_PHASE_IDLE;
static uint32_t g_task3_stop_stable_start_ms = 0U;
static uint32_t g_task3_post_b_start_ms = 0U;
static uint8_t g_task3_stop_stable_active = 0U;
static float g_task3_previous_speed_command_mm_s = 0.0f;
static volatile task4_phase_t g_task4_phase = TASK4_PHASE_IDLE;
static uint32_t g_task4_stop_stable_start_ms = 0U;
static uint32_t g_task4_stop_start_ms = 0U;
static uint8_t g_task4_stop_stable_active = 0U;
static float g_task4_previous_speed_command_mm_s = 0.0f;
static volatile task5_phase_t g_task5_phase = TASK5_PHASE_IDLE;
static uint32_t g_task5_stop_stable_start_ms = 0U;
static uint32_t g_task5_stop_start_ms = 0U;
static uint32_t g_task5_target_average_start_ms = 0U;
static uint32_t g_task5_last_sample_frame_count = 0U;
static uint32_t g_task5_position_sample_count = 0U;
static uint8_t g_task5_stop_stable_active = 0U;
static float g_task5_previous_speed_command_mm_s = 0.0f;
static float g_task5_position_sum_cm = 0.0f;
static float g_task5_target_cm = 0.0f;
static float g_task5_curvature_compensation_deg = 0.0f;

/* 计算浮点数绝对值，避免额外引入数学库。 */
static float competition_tasks_abs_float(float value)
{
    return (value < 0.0f) ? -value : value;
}

/* 清除 Task2 当前目标点的连续稳定计时窗口。 */
static void competition_tasks_reset_task2_window(void)
{
    g_task2_stable_start_ms = 0U;
    g_task2_stable_active = 0U;
}

static void competition_tasks_reset_task3_windows(void)
{
    g_task3_stop_stable_start_ms = 0U;
    g_task3_post_b_start_ms = 0U;
    g_task3_stop_stable_active = 0U;
    g_task3_previous_speed_command_mm_s = 0.0f;
}

static void competition_tasks_reset_task4_windows(void)
{
    g_task4_stop_stable_start_ms = 0U;
    g_task4_stop_start_ms = 0U;
    g_task4_stop_stable_active = 0U;
    g_task4_previous_speed_command_mm_s = 0.0f;
}

static void competition_tasks_reset_task5_windows(void)
{
    g_task5_stop_stable_start_ms = 0U;
    g_task5_stop_start_ms = 0U;
    g_task5_target_average_start_ms = 0U;
    g_task5_last_sample_frame_count = 0U;
    g_task5_position_sample_count = 0U;
    g_task5_stop_stable_active = 0U;
    g_task5_previous_speed_command_mm_s = 0.0f;
    g_task5_position_sum_cm = 0.0f;
    g_task5_target_cm = 0.0f;
    g_task5_curvature_compensation_deg = 0.0f;
}

static bool competition_tasks_task5_frame_to_position(
    const uart_ball_data_t *frame,
    uint32_t reference_ms,
    float *position_cm)
{
    if ((frame == 0) || (position_cm == 0) ||
        (frame->frame_count == 0U) || (frame->valid == 0U))
    {
        return false;
    }

    if ((uint32_t)(reference_ms - frame->last_update_ms) >
        DUOJI_BALL_UART_TIMEOUT_MS)
    {
        return false;
    }

    return duoji_ball_data_to_position_cm(frame, position_cm);
}

static float competition_tasks_task5_curvature_compensation_deg(
    float position_cm)
{
    if (position_cm <= DUOJI_BALL_MIN_POSITION_CM)
    {
        return -TASK5_CURVATURE_NEGATIVE_MAX_DEG;
    }
    if (position_cm >= DUOJI_BALL_MAX_POSITION_CM)
    {
        return TASK5_CURVATURE_POSITIVE_MAX_DEG;
    }

    if (position_cm < 0.0f)
    {
        return position_cm /
               (-DUOJI_BALL_MIN_POSITION_CM) *
               TASK5_CURVATURE_NEGATIVE_MAX_DEG;
    }

    return position_cm /
           DUOJI_BALL_MAX_POSITION_CM *
           TASK5_CURVATURE_POSITIVE_MAX_DEG;
}

static void competition_tasks_set_task5_curvature_compensation(
    float position_cm)
{
    g_task5_curvature_compensation_deg =
        competition_tasks_task5_curvature_compensation_deg(position_cm);
    duoji_ball_control_set_persistent_tilt_deg(
        g_task5_curvature_compensation_deg);
}

static void competition_tasks_start_task5_drive(float target_cm)
{
    g_task5_target_cm = target_cm;
    g_task5_target_average_start_ms = 0U;
    g_task5_last_sample_frame_count = 0U;
    g_task5_position_sample_count = 0U;
    g_task5_previous_speed_command_mm_s = 0.0f;
    g_task5_position_sum_cm = 0.0f;

    line_pid_reset();
    line_pid_set_speed_scale(1.0f);
    motor_reset_odometry();

    duoji_ball_control_set_target_cm(g_task5_target_cm);
    /* 起步前按最终目标重新计算并锁存，行驶后不再更新该补偿。 */
    competition_tasks_set_task5_curvature_compensation(
        g_task5_target_cm);
    duoji_ball_control_reset();
    duoji_ball_control_set_vehicle_acceleration_ratio(1.0f);
    g_task5_phase = TASK5_PHASE_DRIVE;
}

/* 根据相邻两个 10 ms 周期的斜坡速度命令生成 -1～+1 的加速度比例。 */
static void competition_tasks_update_task3_acceleration_feedforward(void)
{
    float current_speed_command_mm_s;
    float speed_command_delta_mm_s;
    float full_step_delta_mm_s;
    float acceleration_ratio = 0.0f;

    current_speed_command_mm_s =
        line_pid_get_current_speed_command_mm_s();
    speed_command_delta_mm_s =
        current_speed_command_mm_s -
        g_task3_previous_speed_command_mm_s;

    if ((speed_command_delta_mm_s > 0.0f) &&
        (g_task3_line_config.max_acceleration > 0.0f))
    {
        full_step_delta_mm_s =
            g_task3_line_config.max_acceleration *
            TASK3_MOTOR_CONTROL_PERIOD_S;
        acceleration_ratio =
            speed_command_delta_mm_s / full_step_delta_mm_s;
    }
    else if ((speed_command_delta_mm_s < 0.0f) &&
             (g_task3_line_config.max_deceleration > 0.0f))
    {
        full_step_delta_mm_s =
            g_task3_line_config.max_deceleration *
            TASK3_MOTOR_CONTROL_PERIOD_S;
        acceleration_ratio =
            speed_command_delta_mm_s / full_step_delta_mm_s;
    }

    duoji_ball_control_set_vehicle_acceleration_ratio(
        acceleration_ratio);
    g_task3_previous_speed_command_mm_s =
        current_speed_command_mm_s;
}

/* 根据 Task4 相邻两个 10 ms 速度命令生成 -1～+1 的加速度比例。 */
static void competition_tasks_update_task4_acceleration_feedforward(void)
{
    float current_speed_command_mm_s;
    float speed_command_delta_mm_s;
    float full_step_delta_mm_s;
    float acceleration_ratio = 0.0f;

    current_speed_command_mm_s =
        line_pid_get_current_speed_command_mm_s();
    speed_command_delta_mm_s =
        current_speed_command_mm_s -
        g_task4_previous_speed_command_mm_s;

    if ((speed_command_delta_mm_s > 0.0f) &&
        (g_task4_line_config.max_acceleration > 0.0f))
    {
        full_step_delta_mm_s =
            g_task4_line_config.max_acceleration *
            TASK4_MOTOR_CONTROL_PERIOD_S;
        acceleration_ratio =
            speed_command_delta_mm_s / full_step_delta_mm_s;
    }
    else if ((speed_command_delta_mm_s < 0.0f) &&
             (g_task4_line_config.max_deceleration > 0.0f))
    {
        full_step_delta_mm_s =
            g_task4_line_config.max_deceleration *
            TASK4_MOTOR_CONTROL_PERIOD_S;
        acceleration_ratio =
            speed_command_delta_mm_s / full_step_delta_mm_s;
    }

    duoji_ball_control_set_vehicle_acceleration_ratio(
        acceleration_ratio);
    g_task4_previous_speed_command_mm_s =
        current_speed_command_mm_s;
}

/* 根据 Task5 相邻两个 10 ms 速度命令生成 -1～+1 的加速度比例。 */
static void competition_tasks_update_task5_acceleration_feedforward(void)
{
    float current_speed_command_mm_s;
    float speed_command_delta_mm_s;
    float full_step_delta_mm_s;
    float acceleration_ratio = 0.0f;

    current_speed_command_mm_s =
        line_pid_get_current_speed_command_mm_s();
    speed_command_delta_mm_s =
        current_speed_command_mm_s -
        g_task5_previous_speed_command_mm_s;

    if ((speed_command_delta_mm_s > 0.0f) &&
        (g_task5_line_config.max_acceleration > 0.0f))
    {
        full_step_delta_mm_s =
            g_task5_line_config.max_acceleration *
            TASK5_MOTOR_CONTROL_PERIOD_S;
        acceleration_ratio =
            speed_command_delta_mm_s / full_step_delta_mm_s;
    }
    else if ((speed_command_delta_mm_s < 0.0f) &&
             (g_task5_line_config.max_deceleration > 0.0f))
    {
        full_step_delta_mm_s =
            g_task5_line_config.max_deceleration *
            TASK5_MOTOR_CONTROL_PERIOD_S;
        acceleration_ratio =
            speed_command_delta_mm_s / full_step_delta_mm_s;
    }

    duoji_ball_control_set_vehicle_acceleration_ratio(
        acceleration_ratio);
    g_task5_previous_speed_command_mm_s =
        current_speed_command_mm_s;
}

static void competition_tasks_fail_task3(void)
{
    /* B 点完成事件可能由高优先级电机中断先触发，完成后不得再改写为失败。 */
    if ((task_manager_get_state() != TASK_STATE_RUNNING) ||
        ((g_task3_phase != TASK3_PHASE_DRIVE) &&
         (g_task3_phase != TASK3_PHASE_APPROACH_B)))
    {
        return;
    }

    g_task3_phase = TASK3_PHASE_FAILED;
    line_pid_set_speed_scale(0.0f);
    duoji_ball_control_set_vehicle_acceleration_ratio(0.0f);
    motor_stop_all();
    task_manager_fail();
}

static void competition_tasks_fail_task4(void)
{
    if ((task_manager_get_state() != TASK_STATE_RUNNING) ||
        ((g_task4_phase != TASK4_PHASE_DRIVE) &&
         (g_task4_phase != TASK4_PHASE_STOPPING)))
    {
        return;
    }

    g_task4_phase = TASK4_PHASE_FAILED;
    line_pid_set_speed_scale(0.0f);
    duoji_ball_control_set_vehicle_acceleration_ratio(0.0f);
    motor_stop_all();
    task_manager_fail();
}

static void competition_tasks_fail_task5(void)
{
    if ((task_manager_get_state() != TASK_STATE_RUNNING) ||
        ((g_task5_phase != TASK5_PHASE_AVERAGE_TARGET) &&
         (g_task5_phase != TASK5_PHASE_DRIVE) &&
         (g_task5_phase != TASK5_PHASE_STOPPING)))
    {
        return;
    }

    g_task5_phase = TASK5_PHASE_FAILED;
    line_pid_set_speed_scale(0.0f);
    duoji_ball_control_set_vehicle_acceleration_ratio(0.0f);
    motor_stop_all();
    task_manager_fail();
}

/* 所有任务启动前都先停车，并将摆杆恢复到 145 度水平位置。 */
static void competition_tasks_stop_car_and_level_servo(void)
{
    motor_stop_all();
    duoji_ball_control_set_vehicle_acceleration_ratio(0.0f);
    duoji_ball_control_set_target_cm(0.0f);
    /* 防止 Task5 的锁存补偿泄漏到 Task1～Task4。 */
    duoji_ball_control_set_persistent_tilt_deg(0.0f);
    duoji_ball_control_reset();
}

/*
 * Task1：小车正常循迹，不运行小球视觉闭环。
 * START 事件负责初始化；MOTOR_10MS 事件负责循迹和车轮速度控制。
 */
static bool task1(competition_task_event_t event)
{
    line_state_t line_state;

    if (event == TASK_EVENT_START)
    {
        competition_tasks_stop_car_and_level_servo();
        g_task2_phase = TASK2_PHASE_IDLE;
        competition_tasks_reset_task2_window();
        g_task3_phase = TASK3_PHASE_IDLE;
        competition_tasks_reset_task3_windows();
        g_task4_phase = TASK4_PHASE_IDLE;
        competition_tasks_reset_task4_windows();
        g_task5_phase = TASK5_PHASE_IDLE;
        competition_tasks_reset_task5_windows();

        /* 加载 Task1 专属参数，再重新开始循迹状态机。 */
        line_pid_set_config(&g_task1_line_config);

        /* 两个车轮均设为正向。 */
        motor_set_direction(1U, 1U);
        motor_set_direction(2U, 1U);
        line_pid_reset();
        line_pid_set_speed_scale(1.0f);
        return false;
    }

    /* Task1 不处理球控中断，非运行状态也不能输出电机控制。 */
    if ((event != TASK_EVENT_MOTOR_10MS) ||
        (task_manager_get_state() != TASK_STATE_RUNNING))
    {
        return false;
    }

    line_pid_update();
    line_state = line_pid_get_state();

    /* 检测终点后继续循迹 0.5 秒，到达停车状态时正常结束计时。 */
    if (line_state == LINE_STATE_FINISH_STOP)
    {
        motor_stop_all();
        task_manager_finish();
        return false;
    }

    /* 丢线搜索超时：立即停车，以失败状态冻结计时。 */
    if (line_state == LINE_STATE_LOST_STOP)
    {
        motor_stop_all();
        task_manager_fail();
        return false;
    }

    /* 循迹仍在进行时，根据目标速度更新左右轮 PWM。 */
    motor_update_speed_pid();
    return false;
}

/*
 * Task2：车辆保持静止，小球先到 +5 cm，再折返到 -5 cm 并保持。
 * 只有 BALL_10MS 事件会执行视觉闭环和稳定时间判断。
 */
static bool task2(competition_task_event_t event)
{
    task_state_t task_state;
    duoji_ball_control_status_t control_status;
    float current_position;
    float current_velocity;
    uint32_t now_ms;

    if (event == TASK_EVENT_START)
    {
        /* 清零两轮输出和球控历史，然后从 +5 cm 目标开始。 */
        competition_tasks_stop_car_and_level_servo();
        duoji_ball_control_set_config(&g_task2_ball_config);
        competition_tasks_reset_task2_window();
        g_task3_phase = TASK3_PHASE_IDLE;
        competition_tasks_reset_task3_windows();
        g_task4_phase = TASK4_PHASE_IDLE;
        competition_tasks_reset_task4_windows();
        g_task5_phase = TASK5_PHASE_IDLE;
        competition_tasks_reset_task5_windows();
        duoji_ball_set_gravity_compensation(false); /* +5cm 阶段：纯 PD */
        duoji_ball_control_set_target_cm(TASK2_POSITIVE_TARGET_CM);
        g_task2_phase = TASK2_PHASE_MOVE_POSITIVE;
        return false;
    }

    if (event != TASK_EVENT_BALL_10MS)
    {
        return false;
    }

    task_state = task_manager_get_state();
    if ((task_state != TASK_STATE_RUNNING) &&
        (task_state != TASK_STATE_FINISHED))
    {
        return false;
    }

    /*
     * 新且有效的视觉帧才会推进判断。
     * 重复帧不累计稳定时间，但舵机内部仍会推进定时脉冲；
     * 无球、超时或标定无效会清除稳定窗口。
     */
    if (!duoji_ball_control_update())
    {
        control_status = duoji_ball_control_get_status();
        if (control_status != DUOJI_CTRL_DUPLICATE_FRAME)
        {
            competition_tasks_reset_task2_window();
        }
        return false;
    }

    /* 完成后仍执行上面的舵机闭环，但不再改变任务阶段和计时。 */
    if ((task_state != TASK_STATE_RUNNING) ||
        (g_task2_phase == TASK2_PHASE_HOLD_NEGATIVE))
    {
        return false;
    }

    current_position = duoji_ball_control_get_position_cm();
    current_velocity = duoji_ball_control_get_velocity_cm_s();
    now_ms = sys_tick_ms;

    if (g_task2_phase == TASK2_PHASE_MOVE_POSITIVE)
    {
        /*
         * +5 cm 阶段：球进入 [4.0, 6.0] cm 范围 且 速度 < 3 cm/s
         * （不是高速冲过，而是真正"到达"），立即折返到 -5 cm。
         * 如果球冲过头（>6cm），PD 会推回来，等它自然回落到范围内再触发。
         */
        if (current_position > TASK2_POSITIVE_PASS_THRESHOLD_CM &&
            current_position <
                (TASK2_POSITIVE_TARGET_CM + TASK2_NEGATIVE_TOLERANCE_CM) &&
            competition_tasks_abs_float(current_velocity) < 3.0f)
        {
            competition_tasks_reset_task2_window();
            g_task2_phase = TASK2_PHASE_MOVE_NEGATIVE;
            duoji_ball_set_gravity_compensation(true); /* -5cm 阶段：开启重力补偿 */
            duoji_ball_control_set_target_cm(TASK2_NEGATIVE_TARGET_CM);
        }
        return false;
    }

    /* ── MOVE_NEGATIVE 阶段：需要球在 -5cm 附近稳定 ── */
    if (g_task2_phase != TASK2_PHASE_MOVE_NEGATIVE)
    {
        competition_tasks_reset_task2_window();
        return false;
    }

    /*
     * 离开 [-6.0, -4.0] cm 范围，或速度过大，则重新累计稳定时间。
     */
    if (current_position < (TASK2_NEGATIVE_TARGET_CM - TASK2_NEGATIVE_TOLERANCE_CM) ||
        current_position > (TASK2_NEGATIVE_TARGET_CM + TASK2_NEGATIVE_TOLERANCE_CM) ||
        competition_tasks_abs_float(current_velocity) >
            TASK2_VELOCITY_TOLERANCE_CM_S)
    {
        competition_tasks_reset_task2_window();
        return false;
    }

    if (g_task2_stable_active == 0U)
    {
        g_task2_stable_start_ms = now_ms;
        g_task2_stable_active = 1U;
        return false;
    }

    /* 无符号时间差兼容 sys_tick_ms 自然回绕。 */
    if ((uint32_t)(now_ms - g_task2_stable_start_ms) <
        TASK2_NEGATIVE_STABLE_MS)
    {
        return false;
    }

    /* -5 cm 稳定满 500 ms 后冻结时间，并继续保持该位置。 */
    competition_tasks_reset_task2_window();
    g_task2_phase = TASK2_PHASE_HOLD_NEGATIVE;
    task_manager_finish();
    return false;
}

static void competition_tasks_task3_ball_update(void)
{
    if ((g_task3_phase == TASK3_PHASE_IDLE) ||
        (g_task3_phase == TASK3_PHASE_FAILED))
    {
        return;
    }

    /* 视觉异常只会使舵机回到水平，不再改变车辆任务状态。 */
    (void)duoji_ball_control_update();
}

static void competition_tasks_task3_motor_update(task_state_t task_state)
{
    line_state_t line_state;
    float average_distance_mm;
    uint32_t now_ms;

    if ((g_task3_phase == TASK3_PHASE_DRIVE) ||
        (g_task3_phase == TASK3_PHASE_APPROACH_B))
    {
        if (task_state != TASK_STATE_RUNNING)
        {
            return;
        }

        line_pid_update();
        competition_tasks_update_task3_acceleration_feedforward();
        line_state = line_pid_get_state();
        if ((line_state == LINE_STATE_LOST_STOP) ||
            (line_state == LINE_STATE_FINISH_STOP))
        {
            competition_tasks_fail_task3();
            return;
        }

        average_distance_mm = motor_get_average_distance_mm();
        if (average_distance_mm >= TASK3_B_DISTANCE_MM)
        {
            /* B 点只冻结比赛计时，车辆继续沿斜坡缓慢减速。 */
            line_pid_set_speed_scale(0.0f);
            g_task3_post_b_start_ms = sys_tick_ms;
            g_task3_stop_stable_active = 0U;
            g_task3_phase = TASK3_PHASE_POST_B_STOP;
            task_manager_finish();
        }
        else if ((g_task3_phase == TASK3_PHASE_DRIVE) &&
                 (average_distance_mm >=
                  TASK3_APPROACH_B_DISTANCE_MM))
        {
            line_pid_set_speed_scale(TASK3_CROSS_B_SPEED_SCALE);
            g_task3_phase = TASK3_PHASE_APPROACH_B;
        }

        motor_update_speed_pid();
        return;
    }

    if (g_task3_phase != TASK3_PHASE_POST_B_STOP)
    {
        return;
    }

    if ((task_state != TASK_STATE_RUNNING) &&
        (task_state != TASK_STATE_FINISHED))
    {
        return;
    }

    line_pid_set_speed_scale(0.0f);
    line_pid_update();
    competition_tasks_update_task3_acceleration_feedforward();
    line_state = line_pid_get_state();
    if ((line_state == LINE_STATE_LOST_STOP) ||
        (line_state == LINE_STATE_FINISH_STOP))
    {
        duoji_ball_control_set_vehicle_acceleration_ratio(0.0f);
        motor_stop_all();
        g_task3_phase = TASK3_PHASE_HOLD_CENTER;
        return;
    }

    motor_update_speed_pid();
    now_ms = sys_tick_ms;

    if ((competition_tasks_abs_float(speed_1) <=
         TASK3_STOP_SPEED_MM_S) &&
        (competition_tasks_abs_float(speed_2) <=
         TASK3_STOP_SPEED_MM_S))
    {
        if (g_task3_stop_stable_active == 0U)
        {
            g_task3_stop_stable_start_ms = now_ms;
            g_task3_stop_stable_active = 1U;
        }
        else if ((uint32_t)(now_ms - g_task3_stop_stable_start_ms) >=
                 TASK3_STOP_STABLE_MS)
        {
            duoji_ball_control_set_vehicle_acceleration_ratio(0.0f);
            motor_stop_all();
            g_task3_phase = TASK3_PHASE_HOLD_CENTER;
            return;
        }
    }
    else
    {
        g_task3_stop_stable_active = 0U;
        g_task3_stop_stable_start_ms = 0U;
    }

    if ((uint32_t)(now_ms - g_task3_post_b_start_ms) >=
        TASK3_POST_B_STOP_TIMEOUT_MS)
    {
        duoji_ball_control_set_vehicle_acceleration_ratio(0.0f);
        motor_stop_all();
        g_task3_phase = TASK3_PHASE_HOLD_CENTER;
    }
}

/*
 * Task3：从 A 到 B 平滑循迹，钢球始终以 0.5 cm 为目标。
 * B 点冻结计时后继续低速减速，停车完成后仍保持钢球闭环。
 */
static bool task3(competition_task_event_t event)
{
    task_state_t task_state;

    if (event == TASK_EVENT_START)
    {
        competition_tasks_stop_car_and_level_servo();
        duoji_ball_control_set_config(&g_task3_ball_config);
        g_task2_phase = TASK2_PHASE_IDLE;
        competition_tasks_reset_task2_window();
        competition_tasks_reset_task3_windows();
        g_task4_phase = TASK4_PHASE_IDLE;
        competition_tasks_reset_task4_windows();
        g_task5_phase = TASK5_PHASE_IDLE;
        competition_tasks_reset_task5_windows();

        line_pid_set_config(&g_task3_line_config);
        motor_set_direction(1U, 1U);
        motor_set_direction(2U, 1U);
        line_pid_reset();
        line_pid_set_speed_scale(1.0f);
        motor_reset_odometry();

        duoji_ball_set_gravity_compensation(true);
        duoji_ball_control_set_target_cm(TASK3_BALL_TARGET_CM);
        duoji_ball_control_reset();
        /* 预置满加速前馈，使第一个球控周期立即给出启动补偿。 */
        duoji_ball_control_set_vehicle_acceleration_ratio(1.0f);
        g_task3_phase = TASK3_PHASE_DRIVE;
        return false;
    }

    task_state = task_manager_get_state();
    if (event == TASK_EVENT_BALL_10MS)
    {
        if ((task_state == TASK_STATE_RUNNING) ||
            (task_state == TASK_STATE_FINISHED))
        {
            competition_tasks_task3_ball_update();
        }
        return false;
    }

    if (event == TASK_EVENT_MOTOR_10MS)
    {
        competition_tasks_task3_motor_update(task_state);
    }

    return false;
}

static void competition_tasks_task4_ball_update(void)
{
    if ((g_task4_phase == TASK4_PHASE_IDLE) ||
        (g_task4_phase == TASK4_PHASE_FAILED))
    {
        return;
    }

    /* 完成后仍维持闭环；视觉异常只使舵机回水平。 */
    (void)duoji_ball_control_update();
}

static void competition_tasks_task4_motor_update(task_state_t task_state)
{
    line_state_t line_state;
    uint32_t now_ms;

    if (g_task4_phase == TASK4_PHASE_DRIVE)
    {
        if (task_state != TASK_STATE_RUNNING)
        {
            return;
        }

        line_pid_update();
        competition_tasks_update_task4_acceleration_feedforward();
        line_state = line_pid_get_state();
        if ((line_state == LINE_STATE_LOST_STOP) ||
            (line_state == LINE_STATE_FINISH_STOP))
        {
            competition_tasks_fail_task4();
            return;
        }

        /* 单次四黑立即锁定 A 点；下一周期开始按 Task4 减速度降速。 */
        if (line_pid_is_finish_marker_detected())
        {
            line_pid_set_speed_scale(0.0f);
            g_task4_stop_start_ms = sys_tick_ms;
            g_task4_stop_stable_start_ms = 0U;
            g_task4_stop_stable_active = 0U;
            g_task4_phase = TASK4_PHASE_STOPPING;
        }

        motor_update_speed_pid();
        return;
    }

    if (g_task4_phase != TASK4_PHASE_STOPPING)
    {
        return;
    }

    if (task_state != TASK_STATE_RUNNING)
    {
        return;
    }

    line_pid_set_speed_scale(0.0f);
    line_pid_update();
    competition_tasks_update_task4_acceleration_feedforward();
    line_state = line_pid_get_state();
    if ((line_state == LINE_STATE_LOST_STOP) ||
        (line_state == LINE_STATE_FINISH_STOP))
    {
        competition_tasks_fail_task4();
        return;
    }

    motor_update_speed_pid();
    now_ms = sys_tick_ms;

    if ((competition_tasks_abs_float(speed_1) <=
         TASK4_STOP_SPEED_MM_S) &&
        (competition_tasks_abs_float(speed_2) <=
         TASK4_STOP_SPEED_MM_S))
    {
        if (g_task4_stop_stable_active == 0U)
        {
            g_task4_stop_stable_start_ms = now_ms;
            g_task4_stop_stable_active = 1U;
        }
        else if ((uint32_t)(now_ms - g_task4_stop_stable_start_ms) >=
                 TASK4_STOP_STABLE_MS)
        {
            duoji_ball_control_set_vehicle_acceleration_ratio(0.0f);
            motor_stop_all();
            g_task4_phase = TASK4_PHASE_HOLD_CENTER;
            task_manager_finish();
            return;
        }
    }
    else
    {
        g_task4_stop_stable_active = 0U;
        g_task4_stop_stable_start_ms = 0U;
    }

    if ((uint32_t)(now_ms - g_task4_stop_start_ms) >=
        TASK4_STOP_TIMEOUT_MS)
    {
        competition_tasks_fail_task4();
    }
}

/* Task4：钢球保持中心，整圈返回 A 点后平滑减速至停车。 */
static bool task4(competition_task_event_t event)
{
    task_state_t task_state;

    if (event == TASK_EVENT_START)
    {
        competition_tasks_stop_car_and_level_servo();
        duoji_ball_control_set_config(&g_task4_ball_config);
        g_task2_phase = TASK2_PHASE_IDLE;
        competition_tasks_reset_task2_window();
        g_task3_phase = TASK3_PHASE_IDLE;
        competition_tasks_reset_task3_windows();
        competition_tasks_reset_task4_windows();
        g_task5_phase = TASK5_PHASE_IDLE;
        competition_tasks_reset_task5_windows();

        line_pid_set_config(&g_task4_line_config);
        motor_set_direction(1U, 1U);
        motor_set_direction(2U, 1U);
        line_pid_reset();
        line_pid_set_speed_scale(1.0f);
        motor_reset_odometry();

        duoji_ball_set_gravity_compensation(true);
        duoji_ball_control_set_target_cm(TASK4_BALL_TARGET_CM);
        duoji_ball_control_reset();
        duoji_ball_control_set_vehicle_acceleration_ratio(1.0f);
        g_task4_phase = TASK4_PHASE_DRIVE;
        return false;
    }

    task_state = task_manager_get_state();
    if (event == TASK_EVENT_BALL_10MS)
    {
        if ((task_state == TASK_STATE_RUNNING) ||
            (task_state == TASK_STATE_FINISHED))
        {
            competition_tasks_task4_ball_update();
        }
        return false;
    }

    if (event == TASK_EVENT_MOTOR_10MS)
    {
        competition_tasks_task4_motor_update(task_state);
    }

    return false;
}

static void competition_tasks_task5_ball_update(task_state_t task_state)
{
    uart_ball_data_t frame;
    float position_cm;
    float average_target_cm;
    uint32_t frame_elapsed_ms;
    uint32_t now_ms;

    if ((g_task5_phase == TASK5_PHASE_IDLE) ||
        (g_task5_phase == TASK5_PHASE_FAILED))
    {
        return;
    }

    now_ms = sys_tick_ms;
    if (g_task5_phase == TASK5_PHASE_AVERAGE_TARGET)
    {
        if (task_state != TASK_STATE_RUNNING)
        {
            return;
        }

        if (UART_get_ball_data(&frame) &&
            (frame.frame_count != g_task5_last_sample_frame_count))
        {
            g_task5_last_sample_frame_count = frame.frame_count;
            frame_elapsed_ms = (uint32_t)(frame.last_update_ms -
                                          g_task5_target_average_start_ms);

            if ((frame_elapsed_ms < TASK5_TARGET_AVERAGE_MS) &&
                competition_tasks_task5_frame_to_position(
                    &frame, now_ms, &position_cm))
            {
                g_task5_position_sum_cm += position_cm;
                g_task5_position_sample_count++;

                /*
                 * 前三秒按累计平均位置实时调角，降低单帧视觉噪声
                 * 引起的舵机抖动；此阶段不运行原 Task5 闭环。
                 */
                average_target_cm =
                    g_task5_position_sum_cm /
                    (float)g_task5_position_sample_count;
                competition_tasks_set_task5_curvature_compensation(
                    average_target_cm);
                duoji_ball_control_hold_persistent_tilt();
            }
            else if ((frame_elapsed_ms >= TASK5_TARGET_AVERAGE_MS) &&
                     (g_task5_position_sample_count == 0U) &&
                     competition_tasks_task5_frame_to_position(
                         &frame, now_ms, &position_cm))
            {
                /* 三秒内零样本：继续停车，首个有效位置到达后再起步。 */
                competition_tasks_start_task5_drive(position_cm);
                return;
            }
        }

        if ((uint32_t)(now_ms - g_task5_target_average_start_ms) <
            TASK5_TARGET_AVERAGE_MS)
        {
            return;
        }

        if (g_task5_position_sample_count == 0U)
        {
            /* 不再因无球失败，保持停车并等待首个有效位置。 */
            return;
        }

        average_target_cm =
            g_task5_position_sum_cm /
            (float)g_task5_position_sample_count;
        competition_tasks_start_task5_drive(average_target_cm);
        return;
    }

    /* 行驶及完成后继续球控；视觉异常时仍保留锁存的弯管补偿。 */
    (void)duoji_ball_control_update();
}

static void competition_tasks_task5_motor_update(task_state_t task_state)
{
    line_state_t line_state;
    uint32_t now_ms;

    if (g_task5_phase == TASK5_PHASE_AVERAGE_TARGET)
    {
        if (task_state != TASK_STATE_RUNNING)
        {
            return;
        }

        motor_stop_all();
        return;
    }

    if (g_task5_phase == TASK5_PHASE_DRIVE)
    {
        if (task_state != TASK_STATE_RUNNING)
        {
            return;
        }

        line_pid_update();
        competition_tasks_update_task5_acceleration_feedforward();
        line_state = line_pid_get_state();
        if ((line_state == LINE_STATE_LOST_STOP) ||
            (line_state == LINE_STATE_FINISH_STOP))
        {
            competition_tasks_fail_task5();
            return;
        }

        /* 单次四黑立即锁定 A 点；下一周期开始按 Task5 减速度降速。 */
        if (line_pid_is_finish_marker_detected())
        {
            line_pid_set_speed_scale(0.0f);
            g_task5_stop_start_ms = sys_tick_ms;
            g_task5_stop_stable_start_ms = 0U;
            g_task5_stop_stable_active = 0U;
            g_task5_phase = TASK5_PHASE_STOPPING;
        }

        motor_update_speed_pid();
        return;
    }

    if (g_task5_phase != TASK5_PHASE_STOPPING)
    {
        return;
    }

    if (task_state != TASK_STATE_RUNNING)
    {
        return;
    }

    line_pid_set_speed_scale(0.0f);
    line_pid_update();
    competition_tasks_update_task5_acceleration_feedforward();
    line_state = line_pid_get_state();
    if ((line_state == LINE_STATE_LOST_STOP) ||
        (line_state == LINE_STATE_FINISH_STOP))
    {
        competition_tasks_fail_task5();
        return;
    }

    motor_update_speed_pid();
    now_ms = sys_tick_ms;

    if ((competition_tasks_abs_float(speed_1) <=
         TASK5_STOP_SPEED_MM_S) &&
        (competition_tasks_abs_float(speed_2) <=
         TASK5_STOP_SPEED_MM_S))
    {
        if (g_task5_stop_stable_active == 0U)
        {
            g_task5_stop_stable_start_ms = now_ms;
            g_task5_stop_stable_active = 1U;
        }
        else if ((uint32_t)(now_ms - g_task5_stop_stable_start_ms) >=
                 TASK5_STOP_STABLE_MS)
        {
            duoji_ball_control_set_vehicle_acceleration_ratio(0.0f);
            motor_stop_all();
            g_task5_phase = TASK5_PHASE_HOLD_TARGET;
            task_manager_finish();
            return;
        }
    }
    else
    {
        g_task5_stop_stable_active = 0U;
        g_task5_stop_stable_start_ms = 0U;
    }

    if ((uint32_t)(now_ms - g_task5_stop_start_ms) >=
        TASK5_STOP_TIMEOUT_MS)
    {
        competition_tasks_fail_task5();
    }
}

/* Task5：停车采样 3 秒并保持平均位置，整圈返回 A 点后平滑停车。 */
static bool task5(competition_task_event_t event)
{
    uart_ball_data_t current_frame = {0U, 0, 0, 0, 0, 0, 0, 0U, 0U};
    task_state_t task_state;

    if (event == TASK_EVENT_START)
    {
        competition_tasks_stop_car_and_level_servo();
        duoji_ball_control_set_config(&g_task5_ball_config);
        g_task2_phase = TASK2_PHASE_IDLE;
        competition_tasks_reset_task2_window();
        g_task3_phase = TASK3_PHASE_IDLE;
        competition_tasks_reset_task3_windows();
        g_task4_phase = TASK4_PHASE_IDLE;
        competition_tasks_reset_task4_windows();
        competition_tasks_reset_task5_windows();

        line_pid_set_config(&g_task5_line_config);
        motor_set_direction(1U, 1U);
        motor_set_direction(2U, 1U);
        line_pid_reset();
        line_pid_set_speed_scale(0.0f);
        motor_reset_odometry();

        duoji_ball_set_gravity_compensation(true);
        duoji_ball_control_set_target_cm(0.0f);
        duoji_ball_control_reset();
        duoji_ball_control_set_vehicle_acceleration_ratio(0.0f);

        /* 当前帧仅作为采样基线；只统计停车初始化完成后的新视觉帧。 */
        (void)UART_get_ball_data(&current_frame);
        g_task5_last_sample_frame_count = current_frame.frame_count;
        g_task5_target_average_start_ms = sys_tick_ms;
        g_task5_phase = TASK5_PHASE_AVERAGE_TARGET;
        return false;
    }

    task_state = task_manager_get_state();
    if (event == TASK_EVENT_BALL_10MS)
    {
        if ((task_state == TASK_STATE_RUNNING) ||
            (task_state == TASK_STATE_FINISHED))
        {
            competition_tasks_task5_ball_update(task_state);
        }
        return false;
    }

    if (event == TASK_EVENT_MOTOR_10MS)
    {
        competition_tasks_task5_motor_update(task_state);
    }

    return false;
}

/* 根据当前任务编号，把事件交给对应的任务。 */
static bool competition_tasks_dispatch(
    task_id_t task, competition_task_event_t event)
{
    switch (task)
    {
    case TASK_ID_1:
        return task1(event);

    case TASK_ID_2:
        return task2(event);

    case TASK_ID_3:
        return task3(event);

    case TASK_ID_4:
        return task4(event);

    case TASK_ID_5:
        return task5(event);

    default:
        return false;
    }
}

bool competition_tasks_start(task_id_t task)
{
    return competition_tasks_dispatch(task, TASK_EVENT_START);
}

void competition_tasks_motor_isr_update(void)
{
    /* 始终采集编码器速度和里程，再由当前任务决定是否输出电机 PID。 */
    motor_update_speed_feedback();
    (void)competition_tasks_dispatch(
        task_manager_get_selected(), TASK_EVENT_MOTOR_10MS);
}

void competition_tasks_ball_isr_update(void)
{
    (void)competition_tasks_dispatch(
        task_manager_get_selected(), TASK_EVENT_BALL_10MS);
}

#include "mpu_port.h"
#include "inv_mpu.h"
#include "inv_mpu_dmp_motion_driver.h"
#include <math.h>
#include "ti_msp_dl_config.h"

#define MPU6050_TIMEOUT_MS 100

volatile uint32_t sys_tick_ms = 0;

void mget_ms(unsigned long *time)
{
    if (time)
    {
        *time = sys_tick_ms;
    }
}

int MPU_Write_Len(unsigned char addr, unsigned char reg, unsigned char len, unsigned char *buf)
{
    volatile uint32_t timeout = 100000;
    while (!(DL_I2C_getControllerStatus(MPU6050_INST) & DL_I2C_CONTROLLER_STATUS_IDLE))
    {
        if (--timeout == 0)
            return -1;
    }

    DL_I2C_transmitControllerData(MPU6050_INST, reg);
    DL_I2C_startControllerTransfer(MPU6050_INST, addr, DL_I2C_CONTROLLER_DIRECTION_TX, len + 1);

    for (uint16_t i = 0; i < len; i++)
    {
        timeout = 100000;
        while (DL_I2C_isControllerTXFIFOFull(MPU6050_INST))
        {
            if (--timeout == 0)
                return -2;
        }
        DL_I2C_transmitControllerData(MPU6050_INST, buf[i]);
    }

    timeout = 100000;
    while (DL_I2C_getControllerStatus(MPU6050_INST) & DL_I2C_CONTROLLER_STATUS_BUSY)
    {
        if (--timeout == 0)
            return -3;
    }
    timeout = 100000;
    while (!(DL_I2C_getControllerStatus(MPU6050_INST) & DL_I2C_CONTROLLER_STATUS_IDLE))
    {
        if (--timeout == 0)
            return -4;
    }
    return 0;
}

int MPU_Read_Len(unsigned char addr, unsigned char reg, unsigned char len, unsigned char *buf)
{
    volatile uint32_t timeout;

    timeout = 100000;
    while (!(DL_I2C_getControllerStatus(MPU6050_INST) & DL_I2C_CONTROLLER_STATUS_IDLE))
    {
        if (--timeout == 0)
            return -1;
    }

    DL_I2C_transmitControllerData(MPU6050_INST, reg);
    DL_I2C_startControllerTransfer(MPU6050_INST, addr, DL_I2C_CONTROLLER_DIRECTION_TX, 1);

    timeout = 100000;
    while (DL_I2C_getControllerStatus(MPU6050_INST) & DL_I2C_CONTROLLER_STATUS_BUSY)
    {
        if (--timeout == 0)
            return -2;
    }
    timeout = 100000;
    while (!(DL_I2C_getControllerStatus(MPU6050_INST) & DL_I2C_CONTROLLER_STATUS_IDLE))
    {
        if (--timeout == 0)
            return -3;
    }

    DL_I2C_startControllerTransfer(MPU6050_INST, addr, DL_I2C_CONTROLLER_DIRECTION_RX, len);

    for (uint16_t i = 0; i < len; i++)
    {
        timeout = 100000;
        while (DL_I2C_isControllerRXFIFOEmpty(MPU6050_INST))
        {

            if (DL_I2C_getControllerStatus(MPU6050_INST) & DL_I2C_CONTROLLER_STATUS_ERROR)
                return -4;

            if (--timeout == 0)
                return -5;
        }
        buf[i] = DL_I2C_receiveControllerData(MPU6050_INST);
    }

    timeout = 100000;
    while (DL_I2C_getControllerStatus(MPU6050_INST) & DL_I2C_CONTROLLER_STATUS_BUSY)
    {
        if (--timeout == 0)
            return -6;
    }
    timeout = 100000;
    while (!(DL_I2C_getControllerStatus(MPU6050_INST) & DL_I2C_CONTROLLER_STATUS_IDLE))
    {
        if (--timeout == 0)
            return -7;
    }
    return 0;
}

static signed char gyro_orientation[9] = {1, 0, 0,
                                          0, 1, 0,
                                          0, 0, 1};
unsigned short inv_row_2_scale(const signed char *row)
{
    unsigned short b;
    if (row[0] > 0)
        b = 0;
    else if (row[0] < 0)
        b = 4;
    else if (row[1] > 0)
        b = 1;
    else if (row[1] < 0)
        b = 5;
    else if (row[2] > 0)
        b = 2;
    else if (row[2] < 0)
        b = 6;
    else
        b = 7;
    return b;
}
unsigned short inv_orientation_matrix_to_scalar(const signed char *mtx)
{
    unsigned short scalar;
    scalar = inv_row_2_scale(mtx);
    scalar |= inv_row_2_scale(mtx + 3) << 3;
    scalar |= inv_row_2_scale(mtx + 6) << 6;
    return scalar;
}

int DMP_Init(void)
{
    int res;
    SysTick->LOAD = (CPUCLK_FREQ / 1000) - 1;
    SysTick->VAL = 0;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
    __enable_irq();
    delay_cycles(CPUCLK_FREQ / 10);
    res = mpu_init();
    if (res)
        return res;

    mpu_set_sensors(INV_XYZ_GYRO | INV_XYZ_ACCEL);
    mpu_configure_fifo(INV_XYZ_GYRO | INV_XYZ_ACCEL);
    mpu_set_sample_rate(100);

    res = dmp_load_motion_driver_firmware();
    if (res)
        return res;

    dmp_set_orientation(inv_orientation_matrix_to_scalar(gyro_orientation));

    dmp_enable_feature(DMP_FEATURE_6X_LP_QUAT | DMP_FEATURE_TAP |
                       DMP_FEATURE_ANDROID_ORIENT | DMP_FEATURE_SEND_RAW_ACCEL |
                       DMP_FEATURE_SEND_CAL_GYRO | DMP_FEATURE_GYRO_CAL);

    dmp_set_fifo_rate(100);
    res = mpu_set_dmp_state(1);

    return res;
}

#define q30 1073741824.0f
int DMP_Read_Data(float *pitch, float *roll, float *yaw)
{
    short gyro[3], accel[3], sensors;
    unsigned char more;
    long quat[4];

    if (dmp_read_fifo(gyro, accel, quat, NULL, &sensors, &more) == 0)
    {
        if (sensors & INV_WXYZ_QUAT)
        {
            float q0 = quat[0] / q30;
            float q1 = quat[1] / q30;
            float q2 = quat[2] / q30;
            float q3 = quat[3] / q30;

            *pitch = asin(-2 * q1 * q3 + 2 * q0 * q2) * 57.3f;
            *roll = atan2(2 * q2 * q3 + 2 * q0 * q1, -2 * q1 * q1 - 2 * q2 * q2 + 1) * 57.3f;
            *yaw = atan2(2 * (q1 * q2 + q0 * q3), q0 * q0 + q1 * q1 - q2 * q2 - q3 * q3) * 57.3f;
            return 0;
        }
    }
    return -1;
}

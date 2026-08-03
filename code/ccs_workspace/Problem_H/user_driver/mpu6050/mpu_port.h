#ifndef _MPU_PORT_H_
#define _MPU_PORT_H_

#include "ti_msp_dl_config.h"
#include <stdint.h>

// 官方库需要的 4 个底层函数声明
int MPU_Write_Len(unsigned char addr, unsigned char reg, unsigned char len, unsigned char *buf);
int MPU_Read_Len(unsigned char addr, unsigned char reg, unsigned char len, unsigned char *buf);
void mget_ms(unsigned long *time);

// 我们自己封装的 DMP 初始化和读取函数
int DMP_Init(void);
int DMP_Read_Data(float *pitch, float *roll, float *yaw);

#endif

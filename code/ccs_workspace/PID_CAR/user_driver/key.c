#include "key.h"
extern int status;

uint32_t counter_1_A = 0;
uint32_t counter_2_A = 0;
void GROUP1_IRQHandler()
{
    switch (DL_GPIO_getPendingInterrupt(GPIOB))
    {
    case DC_MOTOR_BA_IIDX:
        counter_2_A++;
        break;

    default:
        break;
    }

    // 轮胎转一圈编码器线数 MOTOR_ENCODER 390 次中断
    switch (DL_GPIO_getPendingInterrupt(GPIOA))
    {
    case DC_MOTOR_AA_IIDX:
        counter_1_A++;
        break;

    default:
        break;
    }
}

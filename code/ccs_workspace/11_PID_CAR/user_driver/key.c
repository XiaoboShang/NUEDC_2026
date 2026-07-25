#include "key.h"
extern int status;

uint8_t get_key_state(uint32_t key)
{
    uint8_t high_bits = DL_GPIO_readPins(KEY_PORT, key);
    if ((high_bits & key) != 0)
        return 1;
    else
        return 0;
}

uint32_t counter_1_A = 0;
uint32_t counter_2_A = 0;
void GROUP1_IRQHandler()
{
    switch (DL_GPIO_getPendingInterrupt(GPIOB))
    {
    case KEY_KEY9_IIDX:
        status = (status + 1) % 3;
        break;
    case KEY_KEY10_IIDX:
        status = (status + 3 - 1) % 3;
        break;
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

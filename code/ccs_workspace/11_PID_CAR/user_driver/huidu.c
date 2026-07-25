#include "huidu.h"

uint8_t huidu_value[] = {0, 0, 0, 0, 0};

uint8_t get_gpio_state(GPIO_Regs *gpio_port, uint32_t gpio)
{
    uint8_t high_bits = DL_GPIO_readPins(gpio_port, gpio);
    if ((high_bits & gpio) != 0)
        return 1;
    else
        return 0;
}

void huidu_get_value()
{
    huidu_value[0] = get_gpio_state(HUIDU_L2_PORT, HUIDU_L2_PIN);
    huidu_value[1] = get_gpio_state(HUIDU_L1_PORT, HUIDU_L1_PIN);
    huidu_value[2] = get_gpio_state(HUIDU_M_PORT, HUIDU_M_PIN);
    huidu_value[3] = get_gpio_state(HUIDU_R1_PORT, HUIDU_R1_PIN);
    huidu_value[4] = get_gpio_state(HUIDU_R2_PORT, HUIDU_R2_PIN);
}

extern float target_speed_1;
extern float target_speed_2;
float target_speed_5[] = {0, 125, 175, 200, 400, 500};
void adjust_motor()
{
    huidu_get_value();
    // 全白
    if (huidu_value[0] == 0 && huidu_value[1] == 0 && huidu_value[2] == 0 && huidu_value[3] == 0 && huidu_value[4] == 0)
    {
        motor_set_direction(1, 1);
        motor_set_direction(2, 1);
        float minspeed = target_speed_1 > target_speed_2 ? target_speed_2 : target_speed_1;
        target_speed_1 = minspeed;
        target_speed_2 = minspeed;
    }
    // 全黑
    else if (huidu_value[0] == 1 && huidu_value[1] == 1 && huidu_value[2] == 1 && huidu_value[3] == 1 && huidu_value[4] == 1)
    {
        target_speed_1 = 0;
        target_speed_2 = 0;
    }
    // 正常中间黑一个
    else if (huidu_value[0] == 0 && huidu_value[1] == 0 && huidu_value[2] == 1 && huidu_value[3] == 0 && huidu_value[4] == 0)
    {
        motor_set_direction(1, 1);
        motor_set_direction(2, 1);
        float minspeed = target_speed_1 > target_speed_2 ? target_speed_2 : target_speed_1;
        target_speed_1 = minspeed;
        target_speed_2 = minspeed;
    }
    // L1黑
    else if (huidu_value[0] == 0 && huidu_value[1] == 1)
    {
        target_speed_1 = target_speed_5[2];
        target_speed_2 = target_speed_5[3];
    }
    // L2 L1都黑
    else if (huidu_value[0] == 1 && huidu_value[1] == 1)
    {
        target_speed_1 = target_speed_5[2];
        target_speed_2 = target_speed_5[4];
    }
    // L2黑 L1白
    else if (huidu_value[0] == 1)
    {
        target_speed_1 = target_speed_5[1];
        target_speed_2 = target_speed_5[5];
    }
    // R1黑
    else if (huidu_value[0] == 0 && huidu_value[1] == 1)
    {
        target_speed_1 = target_speed_5[3];
        target_speed_2 = target_speed_5[2];
    }
    // R2 R1都黑
    else if (huidu_value[0] == 1 && huidu_value[1] == 1)
    {
        target_speed_1 = target_speed_5[4];
        target_speed_2 = target_speed_5[2];
    }
    // R2黑 R1白
    else if (huidu_value[0] == 1)
    {
        target_speed_1 = target_speed_5[5];
        target_speed_2 = target_speed_5[1];
    }
}
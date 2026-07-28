#include "huidu.h"

volatile uint8_t huidu_value[8] = {0, 0, 0, 0, 0, 0, 0, 0};

/**
 * @brief 读取指定 GPIO 引脚的高低电平。
 * @param gpio_port GPIO 端口。
 * @param gpio GPIO 引脚掩码。
 * @return 高电平返回 1，低电平返回 0。
 */
uint8_t huidu_get_gpio_state(GPIO_Regs *gpio_port, uint32_t gpio)
{
    uint32_t high_bits = DL_GPIO_readPins(gpio_port, gpio);
    if ((high_bits & gpio) != 0)
    {
        return 1;
    }
    return 0;
}

/**
 * @brief 读取八路灰度传感器，黑色记为 1，白色记为 0。
 * @param 无。
 * @return 无。结果按 L4、L3、L2、L1、R1、R2、R3、R4 保存。
 */
void huidu_get_value(void)
{
    huidu_value[0] = !huidu_get_gpio_state(HUIDU_PORT, HUIDU_L4_PIN);
    huidu_value[1] = !huidu_get_gpio_state(HUIDU_PORT, HUIDU_L3_PIN);
    huidu_value[2] = !huidu_get_gpio_state(HUIDU_PORT, HUIDU_L2_PIN);
    huidu_value[3] = !huidu_get_gpio_state(HUIDU_PORT, HUIDU_L1_PIN);
    huidu_value[4] = !huidu_get_gpio_state(HUIDU_PORT, HUIDU_R1_PIN);
    huidu_value[5] = !huidu_get_gpio_state(HUIDU_PORT, HUIDU_R2_PIN);
    huidu_value[6] = !huidu_get_gpio_state(HUIDU_PORT, HUIDU_R3_PIN);
    huidu_value[7] = !huidu_get_gpio_state(HUIDU_PORT, HUIDU_R4_PIN);
}

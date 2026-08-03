#ifndef HUIDU_H
#define HUIDU_H

#include "ti_msp_dl_config.h"

extern volatile uint8_t huidu_value[8];

/**
 * @brief 读取八路灰度传感器的数字量结果。
 * @param 无。
 * @return 无。结果保存到 huidu_value[0]～huidu_value[7]。
 */
void huidu_get_value(void);

uint8_t huidu_get_gpio_state(GPIO_Regs *gpio_port, uint32_t gpio);
#endif

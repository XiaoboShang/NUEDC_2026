#include "delay.h"

void delay_ms(uint32_t ms)
{
    uint32_t cycles = (CPUCLK_FREQ / 1000) * ms;
    delay_cycles(cycles);
}

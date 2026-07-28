#include "citie.h"

void judge_ads()
{
    if (huidu_get_gpio_state(XI_PORT, XI_TOUCH_PIN))
    {
        DL_GPIO_setPins(XI_PORT, XI_CITIE_PIN);
    }
    else
    {
        DL_GPIO_clearPins(XI_PORT, XI_CITIE_PIN);
    }
}
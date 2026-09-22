#ifndef DRIVER_TIM_H
#define DRIVER_TIM_H

#include "stm32f411xe.h"

void tim_control_tick_init(void);
uint8_t tim_control_tick_is_pending(void);
void tim_control_tick_clear(void);

#endif

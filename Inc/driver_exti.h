#ifndef DRIVER_EXTI_H
#define DRIVER_EXTI_H

#include "stm32f411xe.h"

void exti_buttons_init(void);
void buttons_scan_tick(void);

#endif

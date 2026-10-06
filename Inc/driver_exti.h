#ifndef DRIVER_EXTI_H
#define DRIVER_EXTI_H

#include "stm32f411xe.h"

/* Event bits returned by exti_buttons_scan_tick() */
#define BTN_EVENT_PARK                   0x01U
#define BTN_EVENT_RAIN                   0x02U
#define BTN_EVENT_REDLIGHT               0x04U
#define BTN_EVENT_BRAKE                  0x08U

void exti_buttons_init(void);
uint8_t exti_buttons_scan_tick(void);
void exti_obstacle_init(void);
uint8_t exti_obstacle_scan_tick(void);

#endif

#ifndef SAFETY_BRAKE_H
#define SAFETY_BRAKE_H

#include "stm32f411xe.h"

void safety_brake_init(void);
void safety_brake_activate(void);
void safety_brake_release(void);
void safety_brake_toggle(void);
uint8_t safety_brake_is_active(void);

#endif

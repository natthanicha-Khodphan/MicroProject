#ifndef DRIVER_ADC_H
#define DRIVER_ADC_H
#include "stm32f411xe.h"
void adc_quad_init(void);
uint16_t adc_get_pot_raw(void);
uint16_t adc_get_ldr_raw(void);
uint16_t adc_get_joy_vrx_raw(void);
uint16_t adc_get_joy_vry_raw(void);
#endif

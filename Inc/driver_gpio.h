#ifndef DRIVER_GPIO_H
#define DRIVER_GPIO_H

#include "stm32f411xe.h"

#define GPIO_PIN_STATE_LOW              0U
#define GPIO_PIN_STATE_HIGH             1U

void gpio_clock_enable(GPIO_TypeDef * const p_port);
void gpio_output_init(GPIO_TypeDef * const p_port, uint32_t const u4t_pin);
void gpio_input_pullup_init(GPIO_TypeDef * const p_port, uint32_t const u4t_pin);
void gpio_analog_init(GPIO_TypeDef * const p_port, uint32_t const u4t_pin);
void gpio_alternate_function_init(GPIO_TypeDef * const p_port, uint32_t const u4t_pin, uint32_t const u4t_af_number);
void gpio_alternate_function_pullup_init(GPIO_TypeDef * const p_port, uint32_t const u4t_pin, uint32_t const u4t_af_number);
void gpio_write_pin(GPIO_TypeDef * const p_port, uint32_t const u4t_pin, uint8_t const u1t_state);
uint8_t gpio_read_pin(GPIO_TypeDef * const p_port, uint32_t const u4t_pin);

#endif

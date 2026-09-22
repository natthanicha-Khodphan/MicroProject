#ifndef DRIVER_OLED_H
#define DRIVER_OLED_H

#include "stm32f411xe.h"

void oled_init(void);
uint8_t oled_is_detected(void);
void oled_clear(void);
void oled_set_cursor(uint8_t const u1t_page, uint8_t const u1t_col);
void oled_write_string(char const * const p_str);
void oled_write_char(char const c);
void oled_write_line(uint8_t const u1t_page, char const * const p_str);

#endif

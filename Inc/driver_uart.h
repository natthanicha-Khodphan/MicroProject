#ifndef DRIVER_UART_H
#define DRIVER_UART_H

#include "stm32f411xe.h"

void uart_init(void);
void uart_send(char const * const p_buffer, uint16_t const u2t_length);
void uart_send_string(char const * const p_str);

#endif

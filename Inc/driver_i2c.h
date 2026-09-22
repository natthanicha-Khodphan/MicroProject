#ifndef DRIVER_I2C_H
#define DRIVER_I2C_H

#include "stm32f411xe.h"

void i2c1_init(void);
uint8_t i2c1_probe(uint8_t const u1t_addr);
uint8_t i2c1_write_bytes(uint8_t const u1t_addr, uint8_t const * const p_data, uint16_t const u2t_len);

#endif

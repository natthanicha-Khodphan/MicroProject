#ifndef SENSOR_CONVERT_H
#define SENSOR_CONVERT_H
#include "stm32f411xe.h"
uint16_t sensor_get_closeness_pct(uint16_t const u2t_pot_raw);
uint8_t sensor_is_dark(uint16_t const u2t_ldr_raw);
int16_t sensor_get_joy_speed(uint16_t const u2t_vrx_raw, uint16_t const u2t_vry_raw);
void sensor_joy_calibrate(uint16_t const u2t_vrx_raw, uint16_t const u2t_vry_raw);
uint16_t sensor_get_joy_axis_raw(uint16_t const u2t_vrx_raw, uint16_t const u2t_vry_raw);
uint16_t sensor_get_joy_center(void);
void sensor_ldr_calibrate(uint16_t const u2t_ldr_raw);
uint16_t sensor_get_ldr_threshold(void);
#endif

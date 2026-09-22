#ifndef GEAR_FSM_H
#define GEAR_FSM_H
#include "stm32f411xe.h"
#define GEAR_STATE_PARK                   0U
#define GEAR_STATE_DRIVE                  1U
#define GEAR_STATE_REVERSE                2U
#define GEAR_STATE_PARKED_LOCK            3U
#define MODE_FLAG_RAIN                    0x01U
#define MODE_FLAG_REDLIGHT                0x02U
void gear_fsm_init(void);
void gear_fsm_toggle_park_lock(void);
void gear_fsm_toggle_rain(void);
void gear_fsm_toggle_redlight(void);
void gear_fsm_tick(void);
uint8_t gear_fsm_get_state(void);
uint8_t gear_fsm_get_mode_flags(void);
uint16_t gear_fsm_get_speed(void);
uint16_t gear_fsm_get_gap(void);
#endif

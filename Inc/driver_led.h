#ifndef DRIVER_LED_H
#define DRIVER_LED_H

#include "stm32f411xe.h"

#define LED_ID_HEADLIGHT                 0U   /* PA5 */
#define LED_ID_ALERT                     1U   /* PA6, mirrors buzzer */
#define LED_ID_REVERSE_WARN              2U   /* PA7 */
#define LED_ID_DRIVE_WARN                3U   /* PB6 */

#define LED_STATE_OFF                    0U
#define LED_STATE_ON                     1U

void led_init(void);
void led_set(uint8_t const u1t_led_id, uint8_t const u1t_state);
void led_set_all(uint8_t const u1t_state);

#endif

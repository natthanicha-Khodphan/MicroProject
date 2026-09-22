#include "headlight.h"
#include "board_config.h"
#include "driver_gpio.h"
#include "driver_adc.h"
#include "sensor_convert.h"

void headlight_init(void)
{
    gpio_output_init(BOARD_LED_HEADLIGHT_PORT, BOARD_LED_HEADLIGHT_PIN);
}

void headlight_update(void)
{
    uint16_t u2t_ldr_raw;
    uint8_t u1t_dark;

    u2t_ldr_raw = adc_get_ldr_raw();
    u1t_dark = sensor_is_dark(u2t_ldr_raw);

    if (u1t_dark == 1U)
    {
        gpio_write_pin(BOARD_LED_HEADLIGHT_PORT, BOARD_LED_HEADLIGHT_PIN, GPIO_PIN_STATE_HIGH);
    }
    else
    {
        gpio_write_pin(BOARD_LED_HEADLIGHT_PORT, BOARD_LED_HEADLIGHT_PIN, GPIO_PIN_STATE_LOW);
    }
}

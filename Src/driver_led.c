#include "driver_led.h"
#include "board_config.h"
#include "driver_gpio.h"

void led_init(void)
{
    gpio_output_init(BOARD_LED_HEADLIGHT_PORT, BOARD_LED_HEADLIGHT_PIN);
    gpio_output_init(BOARD_LED_ALERT_PORT, BOARD_LED_ALERT_PIN);
    gpio_output_init(BOARD_LED_REVERSE_WARN_PORT, BOARD_LED_REVERSE_WARN_PIN);
    gpio_output_init(BOARD_LED_DRIVE_WARN_PORT, BOARD_LED_DRIVE_WARN_PIN);
    led_set_all(LED_STATE_OFF);
}

void led_set(uint8_t const u1t_led_id, uint8_t const u1t_state)
{
    uint8_t u1t_level;

    if (u1t_state == LED_STATE_ON)
    {
        u1t_level = GPIO_PIN_STATE_HIGH;
    }
    else
    {
        u1t_level = GPIO_PIN_STATE_LOW;
    }

    if (u1t_led_id == LED_ID_HEADLIGHT)
    {
        gpio_write_pin(BOARD_LED_HEADLIGHT_PORT, BOARD_LED_HEADLIGHT_PIN, u1t_level);
    }
    else if (u1t_led_id == LED_ID_ALERT)
    {
        gpio_write_pin(BOARD_LED_ALERT_PORT, BOARD_LED_ALERT_PIN, u1t_level);
    }
    else if (u1t_led_id == LED_ID_REVERSE_WARN)
    {
        gpio_write_pin(BOARD_LED_REVERSE_WARN_PORT, BOARD_LED_REVERSE_WARN_PIN, u1t_level);
    }
    else if (u1t_led_id == LED_ID_DRIVE_WARN)
    {
        gpio_write_pin(BOARD_LED_DRIVE_WARN_PORT, BOARD_LED_DRIVE_WARN_PIN, u1t_level);
    }
    else
    {
        /* unknown LED id, left empty on purpose */
    }
}

void led_set_all(uint8_t const u1t_state)
{
    led_set(LED_ID_HEADLIGHT, u1t_state);
    led_set(LED_ID_ALERT, u1t_state);
    led_set(LED_ID_REVERSE_WARN, u1t_state);
    led_set(LED_ID_DRIVE_WARN, u1t_state);
}

#include "driver_buzzer.h"
#include "board_config.h"
#include "driver_gpio.h"

/* HW-512 (KY-012) active buzzer: drive pin to the active level to sound */

static void buzzer_write_level(uint8_t const u1t_sound)
{
    uint8_t u1t_level;

    if (BOARD_BUZZER_ACTIVE_LOW == 1U)
    {
        if (u1t_sound == 1U)
        {
            u1t_level = GPIO_PIN_STATE_LOW;
        }
        else
        {
            u1t_level = GPIO_PIN_STATE_HIGH;
        }
    }
    else
    {
        if (u1t_sound == 1U)
        {
            u1t_level = GPIO_PIN_STATE_HIGH;
        }
        else
        {
            u1t_level = GPIO_PIN_STATE_LOW;
        }
    }

    gpio_write_pin(BOARD_BUZZER_PORT, BOARD_BUZZER_PIN, u1t_level);

    if (u1t_sound == 1U)
    {
        gpio_write_pin(BOARD_BUZZER_MIRROR_LED_PORT, BOARD_BUZZER_MIRROR_LED_PIN, GPIO_PIN_STATE_HIGH);
    }
    else
    {
        gpio_write_pin(BOARD_BUZZER_MIRROR_LED_PORT, BOARD_BUZZER_MIRROR_LED_PIN, GPIO_PIN_STATE_LOW);
    }
}

void buzzer_init(void)
{
    gpio_output_init(BOARD_BUZZER_PORT, BOARD_BUZZER_PIN);
    gpio_output_init(BOARD_BUZZER_MIRROR_LED_PORT, BOARD_BUZZER_MIRROR_LED_PIN);
    buzzer_off();
}

void buzzer_on(void)
{
    buzzer_write_level(1U);
}

void buzzer_off(void)
{
    buzzer_write_level(0U);
}

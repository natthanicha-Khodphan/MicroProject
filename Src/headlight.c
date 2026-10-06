#include "headlight.h"
#include "driver_led.h"
#include "driver_adc.h"
#include "sensor_convert.h"

void headlight_init(void)
{
    led_set(LED_ID_HEADLIGHT, LED_STATE_OFF);
}

void headlight_update(void)
{
    uint16_t u2t_ldr_raw;
    uint8_t u1t_dark;

    u2t_ldr_raw = adc_get_ldr_raw();
    u1t_dark = sensor_is_dark(u2t_ldr_raw);

    if (u1t_dark == 1U)
    {
        led_set(LED_ID_HEADLIGHT, LED_STATE_ON);
    }
    else
    {
        led_set(LED_ID_HEADLIGHT, LED_STATE_OFF);
    }
}

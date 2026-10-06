#include "stm32f411xe.h"
#include "driver_led.h"
#include "driver_adc.h"
#include "driver_uart.h"
#include "driver_exti.h"
#include "driver_tim.h"
#include "driver_i2c.h"
#include "driver_oled.h"
#include "driver_buzzer.h"
#include "safety_brake.h"
#include "headlight.h"
#include "gear_fsm.h"
#include "sensor_convert.h"

#define POST_DELAY_COUNT                 300000U
#define ADC_SETTLE_DELAY_COUNT           100000U

static void power_on_self_test(void)
{
    volatile uint32_t u4t_delay;

    /* Send initial banner over UART */
    uart_send_string("\r\n========================================\r\n");
    uart_send_string("   ADAS Simulator v3.1 Initialized      \r\n");
    uart_send_string("   (EXTI buttons, UART TX DMA)          \r\n");
    uart_send_string("========================================\r\n");
    uart_send_string("[BOOT] Hardware Self-Test...\r\n");

    /* Quick blink all 4 LEDs and chirp buzzer (software delay) */
    buzzer_on();
    led_set_all(LED_STATE_ON);

    for (u4t_delay = 0U; u4t_delay < POST_DELAY_COUNT; u4t_delay++)
    {
        /* self-test delay */
    }

    buzzer_off();
    led_set_all(LED_STATE_OFF);

    uart_send_string("[BOOT] LEDs & Buzzer OK\r\n");
}

int main(void)
{
    volatile uint32_t u4t_delay;

    /* LEDs first: buzzer driver mirrors its state on the alert LED */
    led_init();
    safety_brake_init();
    buzzer_init();
    headlight_init();

    /* Initialize UART (TX via DMA) early for console output */
    uart_init();

    /* Visual & Audible hardware confirmation */
    power_on_self_test();

    /* Initialize inputs and peripherals */
    exti_buttons_init();
    exti_obstacle_init();
    adc_quad_init();
    for (u4t_delay = 0U; u4t_delay < ADC_SETTLE_DELAY_COUNT; u4t_delay++)
    {
        /* let DMA fill the ADC buffer */
    }
    sensor_joy_calibrate(adc_get_joy_vrx_raw(), adc_get_joy_vry_raw());
    sensor_ldr_calibrate(adc_get_ldr_raw());
    uart_send_string("[BOOT] ADC (DMA) & Buttons (EXTI) OK (joystick + light calibrated)\r\n");
    uart_send_string("[BOOT] IR obstacle sensor on PB2 (EXTI2)\r\n");

    /* Initialize I2C and OLED */
    i2c1_init();
    oled_init();
    if (oled_is_detected() != 0U)
    {
        uart_send_string("[BOOT] OLED Display OK (SH1106 online)\r\n");
    }
    else
    {
        uart_send_string("[BOOT] OLED not responding (headless mode)\r\n");
    }

    gear_fsm_init();
    tim_control_tick_init();

    uart_send_string("[BOOT] Main loop running at 100 Hz (10ms tick)\r\n\r\n");

    __enable_irq();

    while (1U)
    {
        if (tim_control_tick_is_pending() == 1U)
        {
            tim_control_tick_clear();
            headlight_update();
            gear_fsm_tick();
        }
        else
        {
            /* wait for next timer tick */
        }
    }
}

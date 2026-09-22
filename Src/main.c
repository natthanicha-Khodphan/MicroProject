#include "stm32f411xe.h"
#include "driver_gpio.h"
#include "board_config.h"
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
    uart_send_string("   ADAS Simulator System Initialized    \r\n");
    uart_send_string("========================================\r\n");
    uart_send_string("[BOOT] Hardware Self-Test...\r\n");

    /* Quick blink all 4 LEDs and chirp buzzer (software delay) */
    gpio_write_pin(BOARD_LED_HEADLIGHT_PORT, BOARD_LED_HEADLIGHT_PIN, GPIO_PIN_STATE_HIGH);
    gpio_write_pin(BOARD_LED_DRIVE_WARN_PORT, BOARD_LED_DRIVE_WARN_PIN, GPIO_PIN_STATE_HIGH);
    gpio_write_pin(BOARD_LED_REVERSE_WARN_PORT, BOARD_LED_REVERSE_WARN_PIN, GPIO_PIN_STATE_HIGH);
    gpio_write_pin(BOARD_LED_UNUSED_PORT, BOARD_LED_UNUSED_PIN, GPIO_PIN_STATE_HIGH);
    buzzer_on();

    for (u4t_delay = 0U; u4t_delay < POST_DELAY_COUNT; u4t_delay++)
    {
        /* self-test delay */
    }

    buzzer_off();
    gpio_write_pin(BOARD_LED_HEADLIGHT_PORT, BOARD_LED_HEADLIGHT_PIN, GPIO_PIN_STATE_LOW);
    gpio_write_pin(BOARD_LED_DRIVE_WARN_PORT, BOARD_LED_DRIVE_WARN_PIN, GPIO_PIN_STATE_LOW);
    gpio_write_pin(BOARD_LED_REVERSE_WARN_PORT, BOARD_LED_REVERSE_WARN_PIN, GPIO_PIN_STATE_LOW);
    gpio_write_pin(BOARD_LED_UNUSED_PORT, BOARD_LED_UNUSED_PIN, GPIO_PIN_STATE_LOW);

    uart_send_string("[BOOT] LEDs & Buzzer OK\r\n");
}

int main(void)
{
    volatile uint32_t u4t_delay;

    safety_brake_init();
    buzzer_init();
    headlight_init();

    gpio_output_init(BOARD_LED_UNUSED_PORT, BOARD_LED_UNUSED_PIN);
    gpio_output_init(BOARD_LED_DRIVE_WARN_PORT, BOARD_LED_DRIVE_WARN_PIN);
    gpio_output_init(BOARD_LED_REVERSE_WARN_PORT, BOARD_LED_REVERSE_WARN_PIN);

    /* Initialize UART early for console output */
    uart_init();

    /* Visual & Audible hardware confirmation */
    power_on_self_test();

    /* Initialize inputs and peripherals */
    exti_buttons_init();
    adc_quad_init();
    for (u4t_delay = 0U; u4t_delay < ADC_SETTLE_DELAY_COUNT; u4t_delay++)
    {
        /* let DMA fill the ADC buffer */
    }
    sensor_joy_calibrate(adc_get_joy_vrx_raw(), adc_get_joy_vry_raw());
    sensor_ldr_calibrate(adc_get_ldr_raw());
    uart_send_string("[BOOT] ADC & Buttons OK (joystick + light calibrated)\r\n");

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

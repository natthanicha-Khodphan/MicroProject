#include "driver_exti.h"
#include "board_config.h"
#include "driver_gpio.h"
#include "gear_fsm.h"
#include "safety_brake.h"

/* Debounce threshold: 2 ticks of 10ms = 20ms */
#define DEBOUNCE_TICKS                   2U
#define BTN_EVENT_NONE                   0U
#define BTN_EVENT_PRESSED                1U

static uint8_t gu1t_park_cnt = 0U;
static uint8_t gu1t_park_pressed = 0U;

static uint8_t gu1t_rain_cnt = 0U;
static uint8_t gu1t_rain_pressed = 0U;

static uint8_t gu1t_redlight_cnt = 0U;
static uint8_t gu1t_redlight_pressed = 0U;

static uint8_t gu1t_brake_cnt = 0U;
static uint8_t gu1t_brake_pressed = 0U;

void exti_buttons_init(void)
{
    /* Configure pull-up inputs (buttons are active LOW) */
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

    gpio_input_pullup_init(BOARD_BTN_PARK_PORT, BOARD_BTN_PARK_PIN);
    gpio_input_pullup_init(BOARD_BTN_RAIN_PORT, BOARD_BTN_RAIN_PIN);
    gpio_input_pullup_init(BOARD_BTN_REDLIGHT_PORT, BOARD_BTN_REDLIGHT_PIN);
    gpio_input_pullup_init(BOARD_BTN_BRAKE_PORT, BOARD_BTN_BRAKE_PIN);
    gpio_input_pullup_init(BOARD_JOY_SW_PORT, BOARD_JOY_SW_PIN);
}

static uint8_t button_debounce(GPIO_TypeDef * const p_port, uint32_t const u4t_pin,
                               uint8_t * const p_cnt, uint8_t * const p_pressed)
{
    uint8_t u1t_event;

    u1t_event = BTN_EVENT_NONE;

    if (gpio_read_pin(p_port, u4t_pin) == GPIO_PIN_STATE_LOW)
    {
        if (*p_cnt < DEBOUNCE_TICKS)
        {
            *p_cnt = (uint8_t) (*p_cnt + 1U);
        }
        else if (*p_pressed == 0U)
        {
            *p_pressed = 1U;
            u1t_event = BTN_EVENT_PRESSED;
        }
        else
        {
            /* still held, event already reported */
        }
    }
    else
    {
        *p_cnt = 0U;
        *p_pressed = 0U;
    }

    return u1t_event;
}

void buttons_scan_tick(void)
{
    /* Button 1: PA10 (Park lock toggle) */
    if (button_debounce(BOARD_BTN_PARK_PORT, BOARD_BTN_PARK_PIN,
                        &gu1t_park_cnt, &gu1t_park_pressed) == BTN_EVENT_PRESSED)
    {
        gear_fsm_toggle_park_lock();
    }
    else
    {
        /* no event */
    }

    /* Button 2: PB3 (Rain mode toggle) */
    if (button_debounce(BOARD_BTN_RAIN_PORT, BOARD_BTN_RAIN_PIN,
                        &gu1t_rain_cnt, &gu1t_rain_pressed) == BTN_EVENT_PRESSED)
    {
        gear_fsm_toggle_rain();
    }
    else
    {
        /* no event */
    }

    /* Button 3: PB5 (Red light mode toggle) */
    if (button_debounce(BOARD_BTN_REDLIGHT_PORT, BOARD_BTN_REDLIGHT_PIN,
                        &gu1t_redlight_cnt, &gu1t_redlight_pressed) == BTN_EVENT_PRESSED)
    {
        gear_fsm_toggle_redlight();
    }
    else
    {
        /* no event */
    }

    /* Button 4: PB4 (Emergency brake toggle) */
    if (button_debounce(BOARD_BTN_BRAKE_PORT, BOARD_BTN_BRAKE_PIN,
                        &gu1t_brake_cnt, &gu1t_brake_pressed) == BTN_EVENT_PRESSED)
    {
        safety_brake_toggle();
    }
    else
    {
        /* no event */
    }
}

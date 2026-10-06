#include "driver_exti.h"
#include "board_config.h"
#include "driver_gpio.h"

/* Falling edge on a button pin raises EXTI and arms the debounce.
 * The 10ms tick then confirms the pin is still LOW:
 * 2 ticks = 20ms -> one press event, reported once until release.  */
#define DEBOUNCE_TICKS                   2U
#define EXTI_IRQ_PRIORITY                5U
#define EXTI_PORT_CODE_A                 0x0U
#define EXTI_PORT_CODE_B                 0x1U
#define EXTI_PORT_CODE_C                 0x2U
#define EXTI_LINES_PER_EXTICR            4U
#define EXTI_EXTICR_BITS_PER_LINE        4U
#define EXTI_EXTICR_MASK                 0xFU
#define EXTI_LINE_GROUP_9_5_FIRST        5U
#define EXTI_LINE_GROUP_9_5_LAST         9U
#define EXTI_EDGE_FALLING                0U
#define EXTI_EDGE_RISING                 1U

/* Obstacle sensor: edge from EXTI asserts at once (fast reaction),
 * release only after the pin stays inactive 20 ticks = 200ms
 * so the IR output chattering at the range limit is ignored.     */
#define OBSTACLE_RELEASE_TICKS           20U

#define BTN_COUNT                        4U
#define BTN_IDX_PARK                     0U
#define BTN_IDX_RAIN                     1U
#define BTN_IDX_REDLIGHT                 2U
#define BTN_IDX_BRAKE                    3U

typedef struct
{
    GPIO_TypeDef * p_port;
    uint32_t u4t_pin;
    uint8_t u1t_event_bit;
} exti_button_t;

static exti_button_t const gast_buttons[BTN_COUNT] =
{
    { BOARD_BTN_PARK_PORT,     BOARD_BTN_PARK_PIN,     BTN_EVENT_PARK },
    { BOARD_BTN_RAIN_PORT,     BOARD_BTN_RAIN_PIN,     BTN_EVENT_RAIN },
    { BOARD_BTN_REDLIGHT_PORT, BOARD_BTN_REDLIGHT_PIN, BTN_EVENT_REDLIGHT },
    { BOARD_BTN_BRAKE_PORT,    BOARD_BTN_BRAKE_PIN,    BTN_EVENT_BRAKE }
};

static volatile uint8_t gau1t_edge_pending[BTN_COUNT];   /* set by EXTI ISR */
static uint8_t gau1t_debounce_cnt[BTN_COUNT];
static uint8_t gau1t_pressed[BTN_COUNT];

static volatile uint8_t gu1t_obstacle_edge = 0U;          /* set by EXTI ISR */
static uint8_t gu1t_obstacle_present = 0U;
static uint8_t gu1t_obstacle_release_cnt = 0U;

static uint32_t exti_get_port_code(GPIO_TypeDef const * const p_port)
{
    uint32_t u4t_code;

    if (p_port == GPIOA)
    {
        u4t_code = EXTI_PORT_CODE_A;
    }
    else if (p_port == GPIOB)
    {
        u4t_code = EXTI_PORT_CODE_B;
    }
    else
    {
        u4t_code = EXTI_PORT_CODE_C;
    }

    return u4t_code;
}

static IRQn_Type exti_get_irqn(uint32_t const u4t_pin)
{
    IRQn_Type e_irqn;

    if (u4t_pin == 0U)
    {
        e_irqn = EXTI0_IRQn;
    }
    else if (u4t_pin == 1U)
    {
        e_irqn = EXTI1_IRQn;
    }
    else if (u4t_pin == 2U)
    {
        e_irqn = EXTI2_IRQn;
    }
    else if (u4t_pin == 3U)
    {
        e_irqn = EXTI3_IRQn;
    }
    else if (u4t_pin == 4U)
    {
        e_irqn = EXTI4_IRQn;
    }
    else if ((u4t_pin >= EXTI_LINE_GROUP_9_5_FIRST) && (u4t_pin <= EXTI_LINE_GROUP_9_5_LAST))
    {
        e_irqn = EXTI9_5_IRQn;
    }
    else
    {
        e_irqn = EXTI15_10_IRQn;
    }

    return e_irqn;
}

static void exti_line_init(GPIO_TypeDef * const p_port, uint32_t const u4t_pin, uint8_t const u1t_edge)
{
    uint32_t u4t_reg_idx;
    uint32_t u4t_shift;
    uint32_t u4t_exticr;
    uint32_t u4t_line_mask;
    IRQn_Type e_irqn;

    /* Route the pin's port to its EXTI line */
    u4t_reg_idx = u4t_pin / EXTI_LINES_PER_EXTICR;
    u4t_shift = (u4t_pin % EXTI_LINES_PER_EXTICR) * EXTI_EXTICR_BITS_PER_LINE;
    u4t_exticr = SYSCFG->EXTICR[u4t_reg_idx];
    u4t_exticr = u4t_exticr & (~(EXTI_EXTICR_MASK << u4t_shift));
    u4t_exticr = u4t_exticr | (exti_get_port_code(p_port) << u4t_shift);
    SYSCFG->EXTICR[u4t_reg_idx] = u4t_exticr;

    /* One edge only, unmask, clear any stale pending bit */
    u4t_line_mask = 1UL << u4t_pin;
    if (u1t_edge == EXTI_EDGE_RISING)
    {
        EXTI->FTSR = EXTI->FTSR & (~u4t_line_mask);
        EXTI->RTSR = EXTI->RTSR | u4t_line_mask;
    }
    else
    {
        EXTI->RTSR = EXTI->RTSR & (~u4t_line_mask);
        EXTI->FTSR = EXTI->FTSR | u4t_line_mask;
    }
    EXTI->PR = u4t_line_mask;
    EXTI->IMR = EXTI->IMR | u4t_line_mask;

    e_irqn = exti_get_irqn(u4t_pin);
    NVIC_SetPriority(e_irqn, EXTI_IRQ_PRIORITY);
    NVIC_EnableIRQ(e_irqn);
}

void exti_buttons_init(void)
{
    uint32_t u4t_idx;

    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

    /* Joystick switch is read as plain GPIO (no EXTI) */
    gpio_input_pullup_init(BOARD_JOY_SW_PORT, BOARD_JOY_SW_PIN);

    for (u4t_idx = 0U; u4t_idx < BTN_COUNT; u4t_idx++)
    {
        gau1t_edge_pending[u4t_idx] = 0U;
        gau1t_debounce_cnt[u4t_idx] = 0U;
        gau1t_pressed[u4t_idx] = 0U;

        /* Buttons are active LOW: pull-up input */
        gpio_input_pullup_init(gast_buttons[u4t_idx].p_port, gast_buttons[u4t_idx].u4t_pin);
        exti_line_init(gast_buttons[u4t_idx].p_port, gast_buttons[u4t_idx].u4t_pin, EXTI_EDGE_FALLING);
    }
}

void exti_obstacle_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

    gu1t_obstacle_edge = 0U;
    gu1t_obstacle_present = 0U;
    gu1t_obstacle_release_cnt = 0U;

    /* Pull-up keeps the line inactive (no obstacle) if the module is unplugged */
    gpio_input_pullup_init(BOARD_OBSTACLE_PORT, BOARD_OBSTACLE_PIN);

    /* Interrupt on the edge where an obstacle appears */
    if (BOARD_OBSTACLE_ACTIVE_LOW == 1U)
    {
        exti_line_init(BOARD_OBSTACLE_PORT, BOARD_OBSTACLE_PIN, EXTI_EDGE_FALLING);
    }
    else
    {
        exti_line_init(BOARD_OBSTACLE_PORT, BOARD_OBSTACLE_PIN, EXTI_EDGE_RISING);
    }
}

static uint8_t exti_obstacle_pin_active(void)
{
    uint8_t u1t_level;
    uint8_t u1t_active;

    u1t_level = gpio_read_pin(BOARD_OBSTACLE_PORT, BOARD_OBSTACLE_PIN);

    if (BOARD_OBSTACLE_ACTIVE_LOW == 1U)
    {
        if (u1t_level == GPIO_PIN_STATE_LOW)
        {
            u1t_active = 1U;
        }
        else
        {
            u1t_active = 0U;
        }
    }
    else
    {
        if (u1t_level == GPIO_PIN_STATE_HIGH)
        {
            u1t_active = 1U;
        }
        else
        {
            u1t_active = 0U;
        }
    }

    return u1t_active;
}

uint8_t exti_obstacle_scan_tick(void)
{
    if ((gu1t_obstacle_edge == 1U) || (exti_obstacle_pin_active() == 1U))
    {
        /* Edge seen by EXTI or pin still active: obstacle present now */
        gu1t_obstacle_edge = 0U;
        gu1t_obstacle_present = 1U;
        gu1t_obstacle_release_cnt = 0U;
    }
    else if (gu1t_obstacle_present == 1U)
    {
        gu1t_obstacle_release_cnt = (uint8_t) (gu1t_obstacle_release_cnt + 1U);
        if (gu1t_obstacle_release_cnt >= OBSTACLE_RELEASE_TICKS)
        {
            gu1t_obstacle_present = 0U;
            gu1t_obstacle_release_cnt = 0U;
        }
        else
        {
            /* wait until the path stays clear */
        }
    }
    else
    {
        /* no obstacle */
    }

    return gu1t_obstacle_present;
}

static uint8_t button_debounce(uint32_t const u4t_idx)
{
    uint8_t u1t_level;
    uint8_t u1t_event;

    u1t_event = 0U;
    u1t_level = gpio_read_pin(gast_buttons[u4t_idx].p_port, gast_buttons[u4t_idx].u4t_pin);

    if (gau1t_pressed[u4t_idx] == 1U)
    {
        if (u1t_level == GPIO_PIN_STATE_HIGH)
        {
            /* Released: ready for the next press, drop release-bounce edges */
            gau1t_pressed[u4t_idx] = 0U;
            gau1t_debounce_cnt[u4t_idx] = 0U;
            gau1t_edge_pending[u4t_idx] = 0U;
        }
        else
        {
            /* still held, event already reported */
        }
    }
    else if (gau1t_edge_pending[u4t_idx] == 1U)
    {
        if (u1t_level == GPIO_PIN_STATE_LOW)
        {
            gau1t_debounce_cnt[u4t_idx] = (uint8_t) (gau1t_debounce_cnt[u4t_idx] + 1U);
            if (gau1t_debounce_cnt[u4t_idx] >= DEBOUNCE_TICKS)
            {
                gau1t_pressed[u4t_idx] = 1U;
                gau1t_debounce_cnt[u4t_idx] = 0U;
                gau1t_edge_pending[u4t_idx] = 0U;
                u1t_event = 1U;
            }
            else
            {
                /* keep confirming */
            }
        }
        else
        {
            /* Pin back HIGH before debounce time: glitch, ignore */
            gau1t_debounce_cnt[u4t_idx] = 0U;
            gau1t_edge_pending[u4t_idx] = 0U;
        }
    }
    else
    {
        /* idle: no edge from EXTI */
    }

    return u1t_event;
}

uint8_t exti_buttons_scan_tick(void)
{
    uint32_t u4t_idx;
    uint8_t u1t_events;

    u1t_events = 0U;

    for (u4t_idx = 0U; u4t_idx < BTN_COUNT; u4t_idx++)
    {
        if (button_debounce(u4t_idx) == 1U)
        {
            u1t_events = (uint8_t) (u1t_events | gast_buttons[u4t_idx].u1t_event_bit);
        }
        else
        {
            /* no event for this button */
        }
    }

    return u1t_events;
}

static void exti_service_all(void)
{
    uint32_t u4t_idx;
    uint32_t u4t_line_mask;

    /* Shared handler: check every used line, clear by writing 1 to PR */
    for (u4t_idx = 0U; u4t_idx < BTN_COUNT; u4t_idx++)
    {
        u4t_line_mask = 1UL << gast_buttons[u4t_idx].u4t_pin;
        if ((EXTI->PR & u4t_line_mask) != 0U)
        {
            EXTI->PR = u4t_line_mask;
            gau1t_edge_pending[u4t_idx] = 1U;
        }
        else
        {
            /* this line did not fire */
        }
    }

    u4t_line_mask = 1UL << BOARD_OBSTACLE_PIN;
    if ((EXTI->PR & u4t_line_mask) != 0U)
    {
        EXTI->PR = u4t_line_mask;
        gu1t_obstacle_edge = 1U;
    }
    else
    {
        /* obstacle line did not fire */
    }
}

/* PB2 Obstacle sensor */
void EXTI2_IRQHandler(void)
{
    exti_service_all();
}

/* PB3 Rain */
void EXTI3_IRQHandler(void)
{
    exti_service_all();
}

/* PB4 Emergency brake */
void EXTI4_IRQHandler(void)
{
    exti_service_all();
}

/* PB5 Red light */
void EXTI9_5_IRQHandler(void)
{
    exti_service_all();
}

/* PA10 Park lock */
void EXTI15_10_IRQHandler(void)
{
    exti_service_all();
}

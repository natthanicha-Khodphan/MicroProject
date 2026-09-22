#include "driver_gpio.h"

#define GPIO_MODER_BITS_PER_PIN         2U
#define GPIO_MODER_MASK                 0x3U
#define GPIO_MODER_INPUT_VALUE          0x0U
#define GPIO_MODER_OUTPUT_VALUE         0x1U
#define GPIO_MODER_ANALOG_VALUE         0x3U

#define GPIO_PUPDR_BITS_PER_PIN         2U
#define GPIO_PUPDR_MASK                 0x3U
#define GPIO_PUPDR_PULLUP_VALUE         0x1U
#define GPIO_PUPDR_NONE_VALUE           0x0U

void gpio_clock_enable(GPIO_TypeDef * const p_port)
{
    if (p_port == GPIOA)
    {
        RCC->AHB1ENR = RCC->AHB1ENR | RCC_AHB1ENR_GPIOAEN;
    }
    else if (p_port == GPIOB)
    {
        RCC->AHB1ENR = RCC->AHB1ENR | RCC_AHB1ENR_GPIOBEN;
    }
    else if (p_port == GPIOC)
    {
        RCC->AHB1ENR = RCC->AHB1ENR | RCC_AHB1ENR_GPIOCEN;
    }
    else
    {
        /* unsupported port, left empty on purpose */
    }
}

void gpio_output_init(GPIO_TypeDef * const p_port, uint32_t const u4t_pin)
{
    uint32_t u4t_shift;
    uint32_t u4t_moder;

    gpio_clock_enable(p_port);

    u4t_shift = u4t_pin * GPIO_MODER_BITS_PER_PIN;
    u4t_moder = p_port->MODER;
    u4t_moder = u4t_moder & (~(GPIO_MODER_MASK << u4t_shift));
    u4t_moder = u4t_moder | (GPIO_MODER_OUTPUT_VALUE << u4t_shift);
    p_port->MODER = u4t_moder;
}

void gpio_input_pullup_init(GPIO_TypeDef * const p_port, uint32_t const u4t_pin)
{
    uint32_t u4t_moder_shift;
    uint32_t u4t_moder;
    uint32_t u4t_pupdr_shift;
    uint32_t u4t_pupdr;

    gpio_clock_enable(p_port);

    u4t_moder_shift = u4t_pin * GPIO_MODER_BITS_PER_PIN;
    u4t_moder = p_port->MODER;
    u4t_moder = u4t_moder & (~(GPIO_MODER_MASK << u4t_moder_shift));
    u4t_moder = u4t_moder | (GPIO_MODER_INPUT_VALUE << u4t_moder_shift);
    p_port->MODER = u4t_moder;

    u4t_pupdr_shift = u4t_pin * GPIO_PUPDR_BITS_PER_PIN;
    u4t_pupdr = p_port->PUPDR;
    u4t_pupdr = u4t_pupdr & (~(GPIO_PUPDR_MASK << u4t_pupdr_shift));
    u4t_pupdr = u4t_pupdr | (GPIO_PUPDR_PULLUP_VALUE << u4t_pupdr_shift);
    p_port->PUPDR = u4t_pupdr;
}

void gpio_analog_init(GPIO_TypeDef * const p_port, uint32_t const u4t_pin)
{
    uint32_t u4t_shift;
    uint32_t u4t_moder;

    gpio_clock_enable(p_port);

    u4t_shift = u4t_pin * GPIO_MODER_BITS_PER_PIN;
    u4t_moder = p_port->MODER;
    u4t_moder = u4t_moder & (~(GPIO_MODER_MASK << u4t_shift));
    u4t_moder = u4t_moder | (GPIO_MODER_ANALOG_VALUE << u4t_shift);
    p_port->MODER = u4t_moder;
}

#define GPIO_AF_MODE_VALUE              0x2U
#define GPIO_AFR_BITS_PER_PIN           4U
#define GPIO_AFR_MASK                   0xFU
#define GPIO_AFR_PINS_PER_REGISTER      8U

void gpio_alternate_function_init(GPIO_TypeDef * const p_port, uint32_t const u4t_pin, uint32_t const u4t_af_number)
{
    uint32_t u4t_moder_shift;
    uint32_t u4t_moder;
    uint32_t u4t_afr_index;
    uint32_t u4t_afr_shift;
    uint32_t u4t_afr;

    gpio_clock_enable(p_port);

    u4t_moder_shift = u4t_pin * GPIO_MODER_BITS_PER_PIN;
    u4t_moder = p_port->MODER;
    u4t_moder = u4t_moder & (~(GPIO_MODER_MASK << u4t_moder_shift));
    u4t_moder = u4t_moder | (GPIO_AF_MODE_VALUE << u4t_moder_shift);
    p_port->MODER = u4t_moder;

    if (u4t_pin < GPIO_AFR_PINS_PER_REGISTER)
    {
        u4t_afr_index = 0U;
        u4t_afr_shift = u4t_pin * GPIO_AFR_BITS_PER_PIN;
    }
    else
    {
        u4t_afr_index = 1U;
        u4t_afr_shift = (u4t_pin - GPIO_AFR_PINS_PER_REGISTER) * GPIO_AFR_BITS_PER_PIN;
    }

    u4t_afr = p_port->AFR[u4t_afr_index];
    u4t_afr = u4t_afr & (~(GPIO_AFR_MASK << u4t_afr_shift));
    u4t_afr = u4t_afr | (u4t_af_number << u4t_afr_shift);
    p_port->AFR[u4t_afr_index] = u4t_afr;
}

void gpio_alternate_function_pullup_init(GPIO_TypeDef * const p_port, uint32_t const u4t_pin, uint32_t const u4t_af_number)
{
    uint32_t u4t_pupdr_shift;
    uint32_t u4t_pupdr;

    gpio_alternate_function_init(p_port, u4t_pin, u4t_af_number);

    u4t_pupdr_shift = u4t_pin * GPIO_PUPDR_BITS_PER_PIN;
    u4t_pupdr = p_port->PUPDR;
    u4t_pupdr = u4t_pupdr & (~(GPIO_PUPDR_MASK << u4t_pupdr_shift));
    u4t_pupdr = u4t_pupdr | (GPIO_PUPDR_PULLUP_VALUE << u4t_pupdr_shift);
    p_port->PUPDR = u4t_pupdr;
}

void gpio_write_pin(GPIO_TypeDef * const p_port, uint32_t const u4t_pin, uint8_t const u1t_state)
{
    if (u1t_state == GPIO_PIN_STATE_HIGH)
    {
        p_port->BSRR = (uint32_t) (1UL << u4t_pin);
    }
    else
    {
        p_port->BSRR = (uint32_t) (1UL << (u4t_pin + 16U));
    }
}

uint8_t gpio_read_pin(GPIO_TypeDef * const p_port, uint32_t const u4t_pin)
{
    uint32_t u4t_idr;
    uint8_t u1t_result;

    u4t_idr = p_port->IDR;

    if ((u4t_idr & (1UL << u4t_pin)) != 0U)
    {
        u1t_result = GPIO_PIN_STATE_HIGH;
    }
    else
    {
        u1t_result = GPIO_PIN_STATE_LOW;
    }

    return u1t_result;
}

#include "driver_i2c.h"
#include "board_config.h"
#include "driver_gpio.h"

#define I2C_TIMEOUT_LIMIT                2000U
#define I2C_RESET_DELAY_COUNT            200U
#define I2C_FREQ_16MHZ                   16U
#define I2C_CCR_FAST_MODE                I2C_CCR_FS
#define I2C_CCR_400KHZ                   14U
#define I2C_TRISE_400KHZ                 5U
#define I2C_ADDR_SHIFT                   1U
#define I2C_RESULT_FAIL                  0U
#define I2C_RESULT_OK                    1U

void i2c1_init(void)
{
    volatile uint32_t u4t_delay;

    /* PB8 (SCL), PB9 (SDA) as AF4 with pull-up */
    gpio_alternate_function_pullup_init(BOARD_OLED_SCL_PORT, BOARD_OLED_SCL_PIN, BOARD_OLED_I2C_AF_NUMBER);
    gpio_alternate_function_pullup_init(BOARD_OLED_SDA_PORT, BOARD_OLED_SDA_PIN, BOARD_OLED_I2C_AF_NUMBER);

    /* Open-drain */
    BOARD_OLED_SCL_PORT->OTYPER |= (1UL << BOARD_OLED_SCL_PIN);
    BOARD_OLED_SDA_PORT->OTYPER |= (1UL << BOARD_OLED_SDA_PIN);

    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;

    /* Reset I2C1 to clear any bus lockup */
    RCC->APB1RSTR |= RCC_APB1RSTR_I2C1RST;
    for (u4t_delay = 0U; u4t_delay < I2C_RESET_DELAY_COUNT; u4t_delay++)
    {
        /* reset pulse delay */
    }
    RCC->APB1RSTR &= ~RCC_APB1RSTR_I2C1RST;

    /* Fast mode 400 kHz (APB1 = 16 MHz): CCR = 16M / (3 * 400k) ~ 14 */
    I2C1->CR1 &= ~I2C_CR1_PE;
    I2C1->CR2 = I2C_FREQ_16MHZ;
    I2C1->CCR = I2C_CCR_FAST_MODE | I2C_CCR_400KHZ;
    I2C1->TRISE = I2C_TRISE_400KHZ;
    I2C1->CR1 |= I2C_CR1_PE;
}

static uint8_t i2c1_wait_flag(volatile uint32_t * const p_reg, uint32_t const u4t_flag)
{
    uint32_t u4t_timeout;
    uint8_t u1t_result;

    u4t_timeout = 0U;
    u1t_result = I2C_RESULT_OK;

    while ((((*p_reg) & u4t_flag) == 0U) && (u1t_result == I2C_RESULT_OK))
    {
        if ((I2C1->SR1 & (I2C_SR1_AF | I2C_SR1_BERR)) != 0U)
        {
            I2C1->SR1 &= ~(I2C_SR1_AF | I2C_SR1_BERR);
            u1t_result = I2C_RESULT_FAIL;
        }
        else if (u4t_timeout >= I2C_TIMEOUT_LIMIT)
        {
            u1t_result = I2C_RESULT_FAIL;
        }
        else
        {
            u4t_timeout++;
        }
    }

    return u1t_result;
}

static uint8_t i2c1_wait_bus_free(void)
{
    uint32_t u4t_timeout;
    uint8_t u1t_result;

    u4t_timeout = 0U;
    u1t_result = I2C_RESULT_OK;

    while (((I2C1->SR2 & I2C_SR2_BUSY) != 0U) && (u1t_result == I2C_RESULT_OK))
    {
        if (u4t_timeout >= I2C_TIMEOUT_LIMIT)
        {
            u1t_result = I2C_RESULT_FAIL;
        }
        else
        {
            u4t_timeout++;
        }
    }

    return u1t_result;
}

uint8_t i2c1_probe(uint8_t const u1t_addr)
{
    uint32_t u4t_timeout;
    uint8_t u1t_result;

    if (i2c1_wait_bus_free() == I2C_RESULT_FAIL)
    {
        /* Reset I2C1 if stuck */
        i2c1_init();
        u1t_result = I2C_RESULT_FAIL;
    }
    else
    {
        I2C1->CR1 |= I2C_CR1_START;
        if (i2c1_wait_flag(&I2C1->SR1, I2C_SR1_SB) == I2C_RESULT_FAIL)
        {
            I2C1->CR1 |= I2C_CR1_STOP;
            u1t_result = I2C_RESULT_FAIL;
        }
        else
        {
            I2C1->DR = (uint32_t) ((uint32_t) u1t_addr << I2C_ADDR_SHIFT);

            u4t_timeout = 0U;
            while (((I2C1->SR1 & (I2C_SR1_ADDR | I2C_SR1_AF)) == 0U) && (u4t_timeout < I2C_TIMEOUT_LIMIT))
            {
                u4t_timeout++;
            }

            if ((I2C1->SR1 & I2C_SR1_ADDR) != 0U)
            {
                /* ACK: clear ADDR by reading SR1 then SR2 */
                (void) I2C1->SR1;
                (void) I2C1->SR2;
                I2C1->CR1 |= I2C_CR1_STOP;
                u1t_result = I2C_RESULT_OK;
            }
            else
            {
                /* NACK or timeout */
                I2C1->SR1 &= ~I2C_SR1_AF;
                I2C1->CR1 |= I2C_CR1_STOP;
                u1t_result = I2C_RESULT_FAIL;
            }
        }
    }

    return u1t_result;
}

uint8_t i2c1_write_bytes(uint8_t const u1t_addr, uint8_t const * const p_data, uint16_t const u2t_len)
{
    uint16_t u2t_idx;
    uint8_t u1t_result;

    u1t_result = i2c1_wait_bus_free();

    if (u1t_result == I2C_RESULT_OK)
    {
        I2C1->CR1 |= I2C_CR1_START;
        u1t_result = i2c1_wait_flag(&I2C1->SR1, I2C_SR1_SB);
    }
    else
    {
        /* bus stuck, skip */
    }

    if (u1t_result == I2C_RESULT_OK)
    {
        I2C1->DR = (uint32_t) ((uint32_t) u1t_addr << I2C_ADDR_SHIFT);
        u1t_result = i2c1_wait_flag(&I2C1->SR1, I2C_SR1_ADDR);
    }
    else
    {
        /* start failed */
    }

    if (u1t_result == I2C_RESULT_OK)
    {
        (void) I2C1->SR1;
        (void) I2C1->SR2;

        u2t_idx = 0U;
        while ((u2t_idx < u2t_len) && (u1t_result == I2C_RESULT_OK))
        {
            u1t_result = i2c1_wait_flag(&I2C1->SR1, I2C_SR1_TXE);
            if (u1t_result == I2C_RESULT_OK)
            {
                I2C1->DR = p_data[u2t_idx];
                u2t_idx++;
            }
            else
            {
                /* transfer aborted */
            }
        }
    }
    else
    {
        /* address phase failed */
    }

    if (u1t_result == I2C_RESULT_OK)
    {
        (void) i2c1_wait_flag(&I2C1->SR1, I2C_SR1_BTF);
    }
    else
    {
        I2C1->SR1 &= ~I2C_SR1_AF;
    }

    I2C1->CR1 |= I2C_CR1_STOP;

    if (u1t_result == I2C_RESULT_FAIL)
    {
        /* Recover from NACK / stuck BUSY so later frames can still be sent */
        i2c1_init();
    }
    else
    {
        /* transfer complete */
    }

    return u1t_result;
}

#include "driver_uart.h"
#include "board_config.h"
#include "driver_gpio.h"

void uart_init(void)
{
    /* Enable GPIOA and configure PA2 (TX), PA3 (RX) for USART2 (AF7) */
    gpio_alternate_function_init(BOARD_UART_TX_PORT, BOARD_UART_TX_PIN, BOARD_UART_AF_NUMBER);
    gpio_alternate_function_init(BOARD_UART_RX_PORT, BOARD_UART_RX_PIN, BOARD_UART_AF_NUMBER);

    /* Enable USART2 clock */
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

    /* 115200 baud at 16 MHz HSI: BRR = 139 (0x8B) */
    USART2->BRR = BOARD_UART_BRR_VALUE;

    USART2->CR1 &= ~USART_CR1_M;    /* 8 data bits */
    USART2->CR1 &= ~USART_CR1_PCE;  /* No parity */
    USART2->CR2 &= ~USART_CR2_STOP; /* 1 stop bit */
    USART2->CR1 |= USART_CR1_TE;    /* Transmitter enable */
    USART2->CR1 |= USART_CR1_RE;    /* Receiver enable */
    USART2->CR1 |= USART_CR1_UE;    /* USART enable */
}

void uart_send_char(char const c)
{
    while ((USART2->SR & USART_SR_TXE) == 0U)
    {
        /* wait for transmit data register empty */
    }
    USART2->DR = (uint32_t) c;
}

void uart_send(char const * const p_buffer, uint16_t const u2t_length)
{
    uint16_t u2t_idx;

    for (u2t_idx = 0U; u2t_idx < u2t_length; u2t_idx++)
    {
        uart_send_char(p_buffer[u2t_idx]);
    }
}

void uart_send_string(char const * const p_str)
{
    uint16_t u2t_idx;

    u2t_idx = 0U;
    while (p_str[u2t_idx] != '\0')
    {
        uart_send_char(p_str[u2t_idx]);
        u2t_idx++;
    }
}

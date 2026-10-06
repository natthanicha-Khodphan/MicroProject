#include "driver_uart.h"
#include "board_config.h"
#include "driver_gpio.h"

/* TX path (no polling):
 * uart_send() copies bytes into a ring buffer and returns at once.
 * DMA1 Stream6 / Channel 4 (USART2_TX) drains the buffer; the transfer
 * complete interrupt starts the next chunk until the buffer is empty.  */
#define UART_TX_BUF_SIZE                 1024U  /* must be a power of 2, holds all boot text */
#define UART_TX_BUF_MASK                 (UART_TX_BUF_SIZE - 1U)
#define UART_DMA_IRQ_PRIORITY            6U
#define UART_DMA_STREAM6_ALL_FLAGS       (DMA_HIFCR_CTCIF6 | DMA_HIFCR_CHTIF6 | DMA_HIFCR_CTEIF6 \
                                          | DMA_HIFCR_CDMEIF6 | DMA_HIFCR_CFEIF6)

static volatile char gac_tx_buf[UART_TX_BUF_SIZE];
static volatile uint16_t gu2t_tx_head = 0U;      /* next free slot, written by uart_send() */
static volatile uint16_t gu2t_tx_tail = 0U;      /* first unsent byte, written by DMA ISR */
static volatile uint16_t gu2t_tx_dma_len = 0U;   /* bytes in flight, 0 = DMA idle */

static void uart_dma_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;

    /* Disable DMA1 Stream 6 and wait until disabled */
    DMA1_Stream6->CR &= ~DMA_SxCR_EN;
    while ((DMA1_Stream6->CR & DMA_SxCR_EN) != 0U) { }

    DMA1->HIFCR = UART_DMA_STREAM6_ALL_FLAGS;

    /* Channel 4 (USART2_TX), memory-to-peripheral, 8-bit, memory increment,
     * transfer complete + transfer error interrupts                          */
    DMA1_Stream6->CR = DMA_SxCR_CHSEL_2 | DMA_SxCR_DIR_0 | DMA_SxCR_MINC
                        | DMA_SxCR_TCIE | DMA_SxCR_TEIE;
    DMA1_Stream6->PAR = (uint32_t) &USART2->DR;

    NVIC_SetPriority(DMA1_Stream6_IRQn, UART_DMA_IRQ_PRIORITY);
    NVIC_EnableIRQ(DMA1_Stream6_IRQn);
}

/* Call with DMA idle, from the ISR or with interrupts masked */
static void uart_dma_start_next(void)
{
    uint16_t u2t_head;
    uint16_t u2t_tail;
    uint16_t u2t_len;

    u2t_head = gu2t_tx_head;
    u2t_tail = gu2t_tx_tail;

    if (u2t_head == u2t_tail)
    {
        /* buffer empty: DMA stays idle */
        gu2t_tx_dma_len = 0U;
    }
    else
    {
        /* Send one contiguous block; a wrapped remainder goes in the next chunk */
        if (u2t_head > u2t_tail)
        {
            u2t_len = (uint16_t) (u2t_head - u2t_tail);
        }
        else
        {
            u2t_len = (uint16_t) (UART_TX_BUF_SIZE - u2t_tail);
        }

        DMA1->HIFCR = UART_DMA_STREAM6_ALL_FLAGS;
        DMA1_Stream6->M0AR = (uint32_t) &gac_tx_buf[u2t_tail];
        DMA1_Stream6->NDTR = u2t_len;
        gu2t_tx_dma_len = u2t_len;
        DMA1_Stream6->CR |= DMA_SxCR_EN;
    }
}

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
    USART2->CR3 |= USART_CR3_DMAT;  /* TX requests go to DMA */

    uart_dma_init();

    USART2->CR1 |= USART_CR1_TE;    /* Transmitter enable */
    USART2->CR1 |= USART_CR1_RE;    /* Receiver enable */
    USART2->CR1 |= USART_CR1_UE;    /* USART enable */
}

void uart_send(char const * const p_buffer, uint16_t const u2t_length)
{
    uint16_t u2t_idx;
    uint16_t u2t_next;
    uint32_t u4t_primask;

    for (u2t_idx = 0U; u2t_idx < u2t_length; u2t_idx++)
    {
        u2t_next = (uint16_t) ((gu2t_tx_head + 1U) & UART_TX_BUF_MASK);
        if (u2t_next != gu2t_tx_tail)
        {
            gac_tx_buf[gu2t_tx_head] = p_buffer[u2t_idx];
            gu2t_tx_head = u2t_next;
        }
        else
        {
            /* buffer full: byte dropped, never block the control loop */
        }
    }

    /* Kick the DMA if it is idle (masked so the ISR cannot start it twice) */
    u4t_primask = __get_PRIMASK();
    __disable_irq();
    if (gu2t_tx_dma_len == 0U)
    {
        uart_dma_start_next();
    }
    else
    {
        /* DMA busy: the ISR picks up the new bytes */
    }
    __set_PRIMASK(u4t_primask);
}

void uart_send_string(char const * const p_str)
{
    uint16_t u2t_len;

    u2t_len = 0U;
    while (p_str[u2t_len] != '\0')
    {
        u2t_len++;
    }

    uart_send(p_str, u2t_len);
}

void DMA1_Stream6_IRQHandler(void)
{
    if ((DMA1->HISR & (DMA_HISR_TCIF6 | DMA_HISR_TEIF6)) != 0U)
    {
        /* Chunk done (or aborted on error): release it and send the rest */
        DMA1->HIFCR = UART_DMA_STREAM6_ALL_FLAGS;
        gu2t_tx_tail = (uint16_t) ((gu2t_tx_tail + gu2t_tx_dma_len) & UART_TX_BUF_MASK);
        gu2t_tx_dma_len = 0U;
        uart_dma_start_next();
    }
    else
    {
        /* other stream 6 flags are not enabled */
    }
}

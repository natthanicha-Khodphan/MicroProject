#include "driver_adc.h"
#include "board_config.h"
#include "driver_gpio.h"

#define ADC_STARTUP_DELAY_COUNT          1000U
#define ADC_SAMPLE_TIME_SLOW             0x7U
#define ADC_SAMPLE_TIME_SHIFT            3U
#define ADC_SAMPLE_TIME_MASK             0x7U
#define ADC_SQR3_SQ_SHIFT               5U
#define ADC_SQR1_L_FOUR_CONV            0x00300000U
#define DMA_LIFCR_STREAM0_ALL_FLAGS     0x0000003DU

static volatile uint16_t gau2t_adc_buffer[ADC_SCAN_CHANNEL_COUNT];

static void adc_set_sample_time(uint32_t const u4t_channel)
{
    volatile uint32_t * p_smpr;
    uint32_t u4t_local_ch;

    if (u4t_channel < 10U)
    {
        p_smpr = &ADC1->SMPR2;
        u4t_local_ch = u4t_channel;
    }
    else
    {
        p_smpr = &ADC1->SMPR1;
        u4t_local_ch = u4t_channel - 10U;
    }

    *p_smpr = (*p_smpr) & (~(ADC_SAMPLE_TIME_MASK << (u4t_local_ch * ADC_SAMPLE_TIME_SHIFT)));
    *p_smpr = (*p_smpr) | (ADC_SAMPLE_TIME_SLOW << (u4t_local_ch * ADC_SAMPLE_TIME_SHIFT));
}

void adc_quad_init(void)
{
    uint32_t u4t_delay_index;

    gpio_analog_init(BOARD_POT_PORT, BOARD_POT_PIN);
    gpio_analog_init(BOARD_LDR_PORT, BOARD_LDR_PIN);
    gpio_analog_init(BOARD_JOY_VRX_PORT, BOARD_JOY_VRX_PIN);
    gpio_analog_init(BOARD_JOY_VRY_PORT, BOARD_JOY_VRY_PIN);

    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;

    /* Disable DMA2 Stream 0 and wait until disabled */
    DMA2_Stream0->CR &= ~DMA_SxCR_EN;
    while ((DMA2_Stream0->CR & DMA_SxCR_EN) != 0U) { }

    /* Clear all Stream 0 interrupt flags */
    DMA2->LIFCR = DMA_LIFCR_STREAM0_ALL_FLAGS;

    /* Channel 0 (ADC1), 16-bit to 16-bit, Memory increment, Circular mode */
    DMA2_Stream0->CR = DMA_SxCR_MSIZE_0 | DMA_SxCR_PSIZE_0
                        | DMA_SxCR_MINC | DMA_SxCR_CIRC;
    DMA2_Stream0->PAR = (uint32_t) &ADC1->DR;
    DMA2_Stream0->M0AR = (uint32_t) &gau2t_adc_buffer[0];
    DMA2_Stream0->NDTR = ADC_SCAN_CHANNEL_COUNT;
    DMA2_Stream0->CR |= DMA_SxCR_EN;

    adc_set_sample_time(BOARD_POT_ADC_CHANNEL);
    adc_set_sample_time(BOARD_LDR_ADC_CHANNEL);
    adc_set_sample_time(BOARD_JOY_VRX_ADC_CHANNEL);
    adc_set_sample_time(BOARD_JOY_VRY_ADC_CHANNEL);

    ADC1->SQR1 = ADC_SQR1_L_FOUR_CONV;
    ADC1->SQR3 = (BOARD_POT_ADC_CHANNEL << (ADC_SQR3_SQ_SHIFT * 0U))
                 | (BOARD_LDR_ADC_CHANNEL << (ADC_SQR3_SQ_SHIFT * 1U))
                 | (BOARD_JOY_VRX_ADC_CHANNEL << (ADC_SQR3_SQ_SHIFT * 2U))
                 | (BOARD_JOY_VRY_ADC_CHANNEL << (ADC_SQR3_SQ_SHIFT * 3U));

    ADC1->CR1 |= ADC_CR1_SCAN;
    ADC1->CR2 |= ADC_CR2_CONT | ADC_CR2_DMA | ADC_CR2_DDS;
    ADC1->CR2 |= ADC_CR2_ADON;

    for (u4t_delay_index = 0U; u4t_delay_index < ADC_STARTUP_DELAY_COUNT; u4t_delay_index++)
    {
        /* adc stabilization delay */
    }

    ADC1->CR2 |= ADC_CR2_SWSTART;
}

uint16_t adc_get_pot_raw(void)
{
    return gau2t_adc_buffer[ADC_BUF_IDX_POT];
}

uint16_t adc_get_ldr_raw(void)
{
    return gau2t_adc_buffer[ADC_BUF_IDX_LDR];
}

uint16_t adc_get_joy_vrx_raw(void)
{
    return gau2t_adc_buffer[ADC_BUF_IDX_JOY_VRX];
}

uint16_t adc_get_joy_vry_raw(void)
{
    return gau2t_adc_buffer[ADC_BUF_IDX_JOY_VRY];
}


#include "driver_tim.h"
#include "board_config.h"

#define TIM_IRQ_PRIORITY                 4U

static volatile uint8_t gu1t_tick_pending = 0U;

void tim_control_tick_init(void)
{
    RCC->APB1ENR = RCC->APB1ENR | RCC_APB1ENR_TIM2EN;

    TIM2->PSC = BOARD_TIM2_PRESCALER;
    TIM2->ARR = BOARD_TIM2_PERIOD;
    TIM2->CNT = 0U;
    TIM2->DIER = TIM2->DIER | TIM_DIER_UIE;

    NVIC_SetPriority(TIM2_IRQn, TIM_IRQ_PRIORITY);
    NVIC_EnableIRQ(TIM2_IRQn);

    TIM2->CR1 = TIM2->CR1 | TIM_CR1_CEN;
}

uint8_t tim_control_tick_is_pending(void)
{
    return gu1t_tick_pending;
}

void tim_control_tick_clear(void)
{
    gu1t_tick_pending = 0U;
}

void TIM2_IRQHandler(void)
{
    if ((TIM2->SR & TIM_SR_UIF) != 0U)
    {
        TIM2->SR = ~TIM_SR_UIF;
        gu1t_tick_pending = 1U;
    }
    else
    {
        /* spurious interrupt, left empty on purpose */
    }
}

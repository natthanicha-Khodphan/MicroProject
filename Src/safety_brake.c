#include "safety_brake.h"

static volatile uint8_t gu1t_brake_active = 0U;

void safety_brake_init(void)
{
    gu1t_brake_active = 0U;
}

void safety_brake_activate(void)
{
    gu1t_brake_active = 1U;
}

void safety_brake_release(void)
{
    gu1t_brake_active = 0U;
}

void safety_brake_toggle(void)
{
    if (gu1t_brake_active != 0U)
    {
        gu1t_brake_active = 0U;
    }
    else
    {
        gu1t_brake_active = 1U;
    }
}

uint8_t safety_brake_is_active(void)
{
    return gu1t_brake_active;
}

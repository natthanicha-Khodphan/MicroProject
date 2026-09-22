#include "sensor_convert.h"
#include "board_config.h"

#define ADC_RAW_MAX                       4095U
#define PERCENT_MAX                       100U
#define JOY_CAL_MIN                       1024U
#define JOY_CAL_MAX                       3072U
#define LDR_DARK_MARGIN                   600U   /* how much darker than room = "dark" */
#define LDR_HYSTERESIS                    150U
#define LDR_THRESHOLD_MIN                 200U
#define LDR_THRESHOLD_MAX                 3900U

static uint16_t gu2t_joy_center = JOY_CENTER_VALUE;
static uint16_t gu2t_ldr_threshold = BOARD_LDR_DARK_THRESHOLD;
static uint8_t gu1t_is_dark = 0U;

static uint16_t sensor_select_joy_axis(uint16_t const u2t_vrx_raw, uint16_t const u2t_vry_raw)
{
    uint16_t u2t_axis_raw;

    if (BOARD_JOY_FORWARD_IS_VRY == 1U)
    {
        u2t_axis_raw = u2t_vry_raw;
    }
    else
    {
        u2t_axis_raw = u2t_vrx_raw;
    }

    return u2t_axis_raw;
}

uint16_t sensor_get_joy_axis_raw(uint16_t const u2t_vrx_raw, uint16_t const u2t_vry_raw)
{
    return sensor_select_joy_axis(u2t_vrx_raw, u2t_vry_raw);
}

void sensor_joy_calibrate(uint16_t const u2t_vrx_raw, uint16_t const u2t_vry_raw)
{
    uint16_t u2t_axis_raw;

    /* Joystick must be released at boot: its rest value becomes the center */
    u2t_axis_raw = sensor_select_joy_axis(u2t_vrx_raw, u2t_vry_raw);

    if ((u2t_axis_raw >= JOY_CAL_MIN) && (u2t_axis_raw <= JOY_CAL_MAX))
    {
        gu2t_joy_center = u2t_axis_raw;
    }
    else
    {
        gu2t_joy_center = JOY_CENTER_VALUE;
    }
}

uint16_t sensor_get_joy_center(void)
{
    return gu2t_joy_center;
}

uint16_t sensor_get_closeness_pct(uint16_t const u2t_pot_raw)
{
    uint16_t u2t_eff;
    uint32_t u4t_scaled;

    if (BOARD_POT_DIRECTION_INVERTED == 1U)
    {
        u2t_eff = ADC_RAW_MAX - u2t_pot_raw;
    }
    else
    {
        u2t_eff = u2t_pot_raw;
    }

    u4t_scaled = ((uint32_t) u2t_eff * PERCENT_MAX) / ADC_RAW_MAX;
    return (uint16_t) u4t_scaled;
}

void sensor_ldr_calibrate(uint16_t const u2t_ldr_raw)
{
    uint32_t u4t_threshold;

    /* Room light at boot is "bright": dark = ambient +/- margin */
    if (BOARD_LDR_DARK_IS_LOW == 1U)
    {
        if (u2t_ldr_raw > (LDR_DARK_MARGIN + LDR_THRESHOLD_MIN))
        {
            u4t_threshold = (uint32_t) u2t_ldr_raw - LDR_DARK_MARGIN;
        }
        else
        {
            u4t_threshold = LDR_THRESHOLD_MIN;
        }
    }
    else
    {
        u4t_threshold = (uint32_t) u2t_ldr_raw + LDR_DARK_MARGIN;
        if (u4t_threshold > LDR_THRESHOLD_MAX)
        {
            u4t_threshold = LDR_THRESHOLD_MAX;
        }
        else
        {
            /* within range */
        }
    }

    gu2t_ldr_threshold = (uint16_t) u4t_threshold;
    gu1t_is_dark = 0U;
}

uint16_t sensor_get_ldr_threshold(void)
{
    return gu2t_ldr_threshold;
}

uint8_t sensor_is_dark(uint16_t const u2t_ldr_raw)
{
    uint16_t u2t_on_level;
    uint16_t u2t_off_level;

    /* Hysteresis: turn ON past threshold, turn OFF only after coming back
     * LDR_HYSTERESIS counts, so the light does not flicker at the edge   */
    if (BOARD_LDR_DARK_IS_LOW == 1U)
    {
        u2t_on_level = gu2t_ldr_threshold;
        u2t_off_level = (uint16_t) (gu2t_ldr_threshold + LDR_HYSTERESIS);

        if (u2t_ldr_raw < u2t_on_level)
        {
            gu1t_is_dark = 1U;
        }
        else if (u2t_ldr_raw > u2t_off_level)
        {
            gu1t_is_dark = 0U;
        }
        else
        {
            /* inside band: keep previous state */
        }
    }
    else
    {
        u2t_on_level = gu2t_ldr_threshold;
        u2t_off_level = (uint16_t) (gu2t_ldr_threshold - LDR_HYSTERESIS);

        if (u2t_ldr_raw > u2t_on_level)
        {
            gu1t_is_dark = 1U;
        }
        else if (u2t_ldr_raw < u2t_off_level)
        {
            gu1t_is_dark = 0U;
        }
        else
        {
            /* inside band: keep previous state */
        }
    }

    return gu1t_is_dark;
}

int16_t sensor_get_joy_speed(uint16_t const u2t_vrx_raw, uint16_t const u2t_vry_raw)
{
    uint16_t u2t_axis_raw;
    int32_t s4t_centered;
    int16_t s2t_speed;

    u2t_axis_raw = sensor_select_joy_axis(u2t_vrx_raw, u2t_vry_raw);
    s4t_centered = (int32_t) u2t_axis_raw - (int32_t) gu2t_joy_center;

    if ((s4t_centered > (0 - (int32_t) JOY_DEADZONE)) && (s4t_centered < (int32_t) JOY_DEADZONE))
    {
        s2t_speed = 0;
    }
    else
    {
        s2t_speed = (int16_t)(s4t_centered * (int32_t) SPEED_MAX_NORMAL / (int32_t) JOY_CENTER_VALUE);
    }

    return s2t_speed;
}

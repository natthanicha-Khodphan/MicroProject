#include "gear_fsm.h"
#include "board_config.h"
#include "driver_adc.h"
#include "driver_led.h"
#include "driver_uart.h"
#include "driver_oled.h"
#include "driver_buzzer.h"
#include "driver_exti.h"
#include "sensor_convert.h"
#include "safety_brake.h"
#include "util_format.h"

#define BLINK_PERIOD_BASE                 50U
#define BLINK_PERIOD_MIN                  4U
#define BLINK_PERIOD_SLOPE                2U
#define BLINK_DUTY_DIVIDER                2U
#define DASHBOARD_PRINT_DIVIDER           20U   /* 20 * 10ms = 200ms */
#define OLED_LINE_DIVIDER                 2U    /* 1 line every 20ms, full screen every 80ms */
#define OLED_LINE_COUNT                   4U
#define OLED_PAGE_GEAR                    0U
#define OLED_PAGE_SPEED                   2U
#define OLED_PAGE_GAP                     4U
#define OLED_PAGE_MODE                    6U
#define OLED_LINE_IDX_GEAR                0U
#define OLED_LINE_IDX_SPEED               1U
#define OLED_LINE_IDX_GAP                 2U
#define DASHBOARD_BUFFER_SIZE             112U
#define OLED_LINE_BUF_SIZE                24U
#define FIXED3_LEN                        3U
#define FIXED4_LEN                        4U
#define REDLIGHT_BEEP_ON_TICKS            15U   /* 150ms ON  */
#define REDLIGHT_BEEP_PERIOD_TICKS        30U   /* 300ms period */

/* Speed dynamics: speed kept in 0.01 km/h units, updated every 10ms tick.
 * value per tick x 100 ticks/s / 100 = km/h per second                    */
#define SPEED_SCALE                       100U
#define ACCEL_DRIVE_PER_TICK              25U   /* +25 km/h/s  (0-100 in ~4s) */
#define ACCEL_REVERSE_PER_TICK            8U    /* +8  km/h/s  (slow, careful) */
#define DECEL_COAST_PER_TICK              10U   /* -10 km/h/s  (release pedal) */
#define DECEL_BRAKE_PER_TICK              40U   /* -40 km/h/s  (normal braking) */
#define DECEL_EMERGENCY_PER_TICK          80U   /* -80 km/h/s  (AEB / E-brake) */
#define MOVE_DIR_NONE                     0U
#define MOVE_DIR_FORWARD                  1U
#define MOVE_DIR_REVERSE                  2U

static uint8_t gu1t_state = GEAR_STATE_PARK;
static uint8_t gu1t_mode_flags = 0U;
static uint16_t gu2t_speed = 0U;
static uint16_t gu2t_speed_x100 = 0U;
static uint8_t gu1t_move_dir = MOVE_DIR_NONE;
static uint16_t gu2t_gap_pct = 0U;
static uint16_t gu2t_blink_counter = 0U;
static uint16_t gu2t_print_counter = 0U;
static uint16_t gu2t_oled_counter = 0U;
static uint8_t gu1t_oled_line_idx = 0U;
static uint16_t gu2t_buzzer_counter = 0U;
static uint16_t gu2t_redlight_baseline = 0U;
static char gac_uart_buf[DASHBOARD_BUFFER_SIZE];
static char gac_oled_line[OLED_LINE_BUF_SIZE];

void gear_fsm_init(void)
{
    gu1t_state = GEAR_STATE_PARK;
    gu1t_mode_flags = 0U;
    gu2t_speed = 0U;
    gu2t_speed_x100 = 0U;
    gu1t_move_dir = MOVE_DIR_NONE;
    gu2t_gap_pct = 0U;
    gu2t_blink_counter = 0U;
    gu2t_print_counter = 0U;
    gu2t_oled_counter = 0U;
    gu1t_oled_line_idx = 0U;
    gu2t_buzzer_counter = 0U;
    gu2t_redlight_baseline = 0U;
}

void gear_fsm_toggle_park_lock(void)
{
    if (gu1t_state == GEAR_STATE_PARKED_LOCK)
    {
        gu1t_state = GEAR_STATE_PARK;
    }
    else if (gu2t_speed_x100 == 0U)
    {
        /* Like a real car: P-lock only engages when fully stopped */
        gu1t_state = GEAR_STATE_PARKED_LOCK;
        gu1t_move_dir = MOVE_DIR_NONE;
    }
    else
    {
        /* moving: request ignored */
    }
}

void gear_fsm_toggle_rain(void)
{
    gu1t_mode_flags ^= MODE_FLAG_RAIN;
}

void gear_fsm_toggle_redlight(void)
{
    if ((gu1t_mode_flags & MODE_FLAG_REDLIGHT) != 0U)
    {
        gu1t_mode_flags &= (uint8_t) (~MODE_FLAG_REDLIGHT);
        buzzer_off();
        gu2t_buzzer_counter = 0U;
    }
    else
    {
        gu1t_mode_flags |= MODE_FLAG_REDLIGHT;
        gu2t_redlight_baseline = gu2t_gap_pct;
        gu2t_buzzer_counter = 0U;
    }
}

uint8_t gear_fsm_get_state(void)
{
    return gu1t_state;
}

uint8_t gear_fsm_get_mode_flags(void)
{
    return gu1t_mode_flags;
}

uint16_t gear_fsm_get_speed(void)
{
    return gu2t_speed;
}

uint16_t gear_fsm_get_gap(void)
{
    return gu2t_gap_pct;
}

static uint16_t fsm_append_fixed3(char * const p_buf, uint16_t const u2t_off, uint16_t const u2t_value)
{
    char ac_digits[FIXED3_LEN];
    uint16_t u2t_idx;

    util_format_u16_fixed3(u2t_value, ac_digits);
    for (u2t_idx = 0U; u2t_idx < FIXED3_LEN; u2t_idx++)
    {
        p_buf[u2t_off + u2t_idx] = ac_digits[u2t_idx];
    }

    return (uint16_t) (u2t_off + FIXED3_LEN);
}

static uint16_t fsm_append_fixed4(char * const p_buf, uint16_t const u2t_off, uint16_t const u2t_value)
{
    char ac_digits[FIXED4_LEN];
    uint16_t u2t_idx;

    util_format_u16_fixed4(u2t_value, ac_digits);
    for (u2t_idx = 0U; u2t_idx < FIXED4_LEN; u2t_idx++)
    {
        p_buf[u2t_off + u2t_idx] = ac_digits[u2t_idx];
    }

    return (uint16_t) (u2t_off + FIXED4_LEN);
}

static void fsm_handle_buttons(uint8_t const u1t_events)
{
    /* Button 1: Park lock toggle */
    if ((u1t_events & BTN_EVENT_PARK) != 0U)
    {
        gear_fsm_toggle_park_lock();
    }
    else
    {
        /* no event */
    }

    /* Button 2: Rain mode toggle */
    if ((u1t_events & BTN_EVENT_RAIN) != 0U)
    {
        gear_fsm_toggle_rain();
    }
    else
    {
        /* no event */
    }

    /* Button 3: Red light mode toggle */
    if ((u1t_events & BTN_EVENT_REDLIGHT) != 0U)
    {
        gear_fsm_toggle_redlight();
    }
    else
    {
        /* no event */
    }

    /* Button 4: Emergency brake toggle */
    if ((u1t_events & BTN_EVENT_BRAKE) != 0U)
    {
        safety_brake_toggle();
    }
    else
    {
        /* no event */
    }
}

static void fsm_update_warning_led(uint16_t const u2t_gap, uint8_t const u1t_led_id)
{
    uint16_t u2t_caution;
    uint16_t u2t_period;
    uint16_t u2t_half;

    if ((gu1t_mode_flags & MODE_FLAG_RAIN) != 0U)
    {
        u2t_caution = ZONE_CAUTION_RAIN;
    }
    else
    {
        u2t_caution = ZONE_CAUTION_NORMAL;
    }

    if (u2t_gap < u2t_caution)
    {
        led_set(u1t_led_id, LED_STATE_OFF);
        gu2t_blink_counter = 0U;
    }
    else
    {
        if (u2t_gap > ((BLINK_PERIOD_BASE - BLINK_PERIOD_MIN) * BLINK_PERIOD_SLOPE))
        {
            u2t_period = BLINK_PERIOD_MIN;
        }
        else
        {
            u2t_period = BLINK_PERIOD_BASE - (u2t_gap / BLINK_PERIOD_SLOPE);
        }

        if (u2t_period < BLINK_PERIOD_MIN)
        {
            u2t_period = BLINK_PERIOD_MIN;
        }
        else
        {
            /* period within range */
        }

        u2t_half = u2t_period / BLINK_DUTY_DIVIDER;
        gu2t_blink_counter++;

        if (gu2t_blink_counter >= u2t_period)
        {
            gu2t_blink_counter = 0U;
        }
        else
        {
            /* keep counting */
        }

        if (gu2t_blink_counter < u2t_half)
        {
            led_set(u1t_led_id, LED_STATE_ON);
        }
        else
        {
            led_set(u1t_led_id, LED_STATE_OFF);
        }
    }
}

static void fsm_update_redlight_buzzer(uint16_t const u2t_gap)
{
    if ((gu1t_mode_flags & MODE_FLAG_REDLIGHT) != 0U)
    {
        if (gu2t_speed != 0U)
        {
            /* Car is moving: re-arm baseline so the next stop starts fresh */
            gu2t_redlight_baseline = u2t_gap;
        }
        else if (u2t_gap > gu2t_redlight_baseline)
        {
            /* Stopped and lead vehicle is closer: track the closest gap */
            gu2t_redlight_baseline = u2t_gap;
        }
        else
        {
            /* baseline unchanged */
        }

        /* Lead Vehicle Departure Alert (LVDA):
         * stopped (speed == 0) right behind the lead car (closest gap
         * 90-100%), then gap drops below baseline - delta (car pulls away) */
        if ((gu2t_speed == 0U) &&
            (gu2t_redlight_baseline >= REDLIGHT_ARM_GAP_MIN) &&
            (u2t_gap < (gu2t_redlight_baseline - REDLIGHT_BUZZER_DELTA)))
        {
            gu2t_buzzer_counter++;
            if (gu2t_buzzer_counter < REDLIGHT_BEEP_ON_TICKS)
            {
                buzzer_on();
            }
            else if (gu2t_buzzer_counter < REDLIGHT_BEEP_PERIOD_TICKS)
            {
                buzzer_off();
            }
            else
            {
                gu2t_buzzer_counter = 0U;
            }
        }
        else
        {
            buzzer_off();
            gu2t_buzzer_counter = 0U;
        }
    }
    else
    {
        buzzer_off();
        gu2t_buzzer_counter = 0U;
    }
}

static char const * fsm_get_gear_str(void)
{
    char const * p_str;

    if (gu1t_state == GEAR_STATE_DRIVE)
    {
        p_str = "D";
    }
    else if (gu1t_state == GEAR_STATE_REVERSE)
    {
        p_str = "R";
    }
    else if (gu1t_state == GEAR_STATE_PARKED_LOCK)
    {
        p_str = "P-LOCK";
    }
    else
    {
        p_str = "P";
    }

    return p_str;
}

static char const * fsm_get_mode_str(void)
{
    char const * p_str;
    uint8_t u1t_rain;
    uint8_t u1t_red;

    u1t_rain = (uint8_t) (gu1t_mode_flags & MODE_FLAG_RAIN);
    u1t_red = (uint8_t) (gu1t_mode_flags & MODE_FLAG_REDLIGHT);

    if (safety_brake_is_active() == 1U)
    {
        p_str = "BRAKE";
    }
    else if ((u1t_rain != 0U) && (u1t_red != 0U))
    {
        p_str = "RAIN+RED";
    }
    else if (u1t_rain != 0U)
    {
        p_str = "RAIN";
    }
    else if (u1t_red != 0U)
    {
        p_str = "REDLIGHT";
    }
    else
    {
        p_str = "NORMAL";
    }

    return p_str;
}

static void fsm_send_uart_dashboard(uint16_t const u2t_raw_pot, uint16_t const u2t_joy_raw)
{
    uint16_t u2t_off;

    gu2t_print_counter++;

    if (gu2t_print_counter >= DASHBOARD_PRINT_DIVIDER)
    {
        gu2t_print_counter = 0U;

        u2t_off = 0U;
        u2t_off = util_copy_str("SPD: ", gac_uart_buf, u2t_off);
        u2t_off = fsm_append_fixed3(gac_uart_buf, u2t_off, gu2t_speed);
        u2t_off = util_copy_str(" | GEAR: ", gac_uart_buf, u2t_off);
        u2t_off = util_copy_str(fsm_get_gear_str(), gac_uart_buf, u2t_off);
        u2t_off = util_copy_str(" | GAP: ", gac_uart_buf, u2t_off);
        u2t_off = fsm_append_fixed3(gac_uart_buf, u2t_off, gu2t_gap_pct);
        u2t_off = util_copy_str("% | RAW: ", gac_uart_buf, u2t_off);
        u2t_off = fsm_append_fixed4(gac_uart_buf, u2t_off, u2t_raw_pot);
        u2t_off = util_copy_str(" | JOY: ", gac_uart_buf, u2t_off);
        u2t_off = fsm_append_fixed4(gac_uart_buf, u2t_off, u2t_joy_raw);
        u2t_off = util_copy_str(" | LDR: ", gac_uart_buf, u2t_off);
        u2t_off = fsm_append_fixed4(gac_uart_buf, u2t_off, adc_get_ldr_raw());
        u2t_off = util_copy_str("/", gac_uart_buf, u2t_off);
        u2t_off = fsm_append_fixed4(gac_uart_buf, u2t_off, sensor_get_ldr_threshold());
        u2t_off = util_copy_str(" | MODE: ", gac_uart_buf, u2t_off);
        u2t_off = util_copy_str(fsm_get_mode_str(), gac_uart_buf, u2t_off);
        u2t_off = util_copy_str("\r\n", gac_uart_buf, u2t_off);

        /* Non-blocking: queued for DMA, returns immediately */
        uart_send(gac_uart_buf, u2t_off);
    }
    else
    {
        /* not time to print yet */
    }
}

static void fsm_update_oled(void)
{
    uint16_t u2t_off;
    uint8_t u1t_page;

    gu2t_oled_counter++;

    if ((oled_is_detected() == 1U) && (gu2t_oled_counter >= OLED_LINE_DIVIDER))
    {
        gu2t_oled_counter = 0U;
        u2t_off = 0U;

        /* Draw only ONE line per call to keep each tick short */
        if (gu1t_oled_line_idx == OLED_LINE_IDX_GEAR)
        {
            u1t_page = OLED_PAGE_GEAR;
            u2t_off = util_copy_str("GEAR:  ", gac_oled_line, u2t_off);
            u2t_off = util_copy_str(fsm_get_gear_str(), gac_oled_line, u2t_off);
        }
        else if (gu1t_oled_line_idx == OLED_LINE_IDX_SPEED)
        {
            u1t_page = OLED_PAGE_SPEED;
            u2t_off = util_copy_str("SPEED: ", gac_oled_line, u2t_off);
            u2t_off = fsm_append_fixed3(gac_oled_line, u2t_off, gu2t_speed);
            u2t_off = util_copy_str(" km/h", gac_oled_line, u2t_off);
        }
        else if (gu1t_oled_line_idx == OLED_LINE_IDX_GAP)
        {
            u1t_page = OLED_PAGE_GAP;
            u2t_off = util_copy_str("GAP:   ", gac_oled_line, u2t_off);
            u2t_off = fsm_append_fixed3(gac_oled_line, u2t_off, gu2t_gap_pct);
            u2t_off = util_copy_str("%", gac_oled_line, u2t_off);
        }
        else
        {
            u1t_page = OLED_PAGE_MODE;
            u2t_off = util_copy_str("MODE:  ", gac_oled_line, u2t_off);
            u2t_off = util_copy_str(fsm_get_mode_str(), gac_oled_line, u2t_off);
        }

        gac_oled_line[u2t_off] = '\0';
        oled_write_line(u1t_page, gac_oled_line);

        gu1t_oled_line_idx++;
        if (gu1t_oled_line_idx >= OLED_LINE_COUNT)
        {
            gu1t_oled_line_idx = 0U;
        }
        else
        {
            /* next line on next call */
        }
    }
    else
    {
        /* not time to draw, or OLED absent */
    }
}

static uint16_t fsm_ramp(uint16_t const u2t_current, uint16_t const u2t_target,
                         uint16_t const u2t_accel, uint16_t const u2t_decel)
{
    uint16_t u2t_next;

    if (u2t_current < u2t_target)
    {
        if ((u2t_target - u2t_current) > u2t_accel)
        {
            u2t_next = (uint16_t) (u2t_current + u2t_accel);
        }
        else
        {
            u2t_next = u2t_target;
        }
    }
    else if (u2t_current > u2t_target)
    {
        if ((u2t_current - u2t_target) > u2t_decel)
        {
            u2t_next = (uint16_t) (u2t_current - u2t_decel);
        }
        else
        {
            u2t_next = u2t_target;
        }
    }
    else
    {
        u2t_next = u2t_current;
    }

    return u2t_next;
}

static uint16_t fsm_get_gap_limit(void)
{
    uint16_t u2t_speed_max;
    uint16_t u2t_zone_critical;
    uint16_t u2t_limit;

    if ((gu1t_mode_flags & MODE_FLAG_RAIN) != 0U)
    {
        u2t_speed_max = SPEED_MAX_RAIN;
        u2t_zone_critical = ZONE_CRITICAL_RAIN;
    }
    else
    {
        u2t_speed_max = SPEED_MAX_NORMAL;
        u2t_zone_critical = ZONE_CRITICAL_NORMAL;
    }

    /* Closer obstacle -> lower allowed speed, 0 inside critical zone */
    if (gu2t_gap_pct >= u2t_zone_critical)
    {
        u2t_limit = 0U;
    }
    else
    {
        u2t_limit = (uint16_t) (u2t_speed_max - ((gu2t_gap_pct * u2t_speed_max) / u2t_zone_critical));
    }

    return u2t_limit;
}

static uint8_t fsm_is_critical_zone(void)
{
    uint16_t u2t_zone_critical;
    uint8_t u1t_critical;

    if ((gu1t_mode_flags & MODE_FLAG_RAIN) != 0U)
    {
        u2t_zone_critical = ZONE_CRITICAL_RAIN;
    }
    else
    {
        u2t_zone_critical = ZONE_CRITICAL_NORMAL;
    }

    if (gu2t_gap_pct >= u2t_zone_critical)
    {
        u1t_critical = 1U;
    }
    else
    {
        u1t_critical = 0U;
    }

    return u1t_critical;
}

static void fsm_update_speed(int16_t const s2t_joy_speed)
{
    uint8_t u1t_req_dir;
    uint16_t u2t_joy_abs;
    uint16_t u2t_target;
    uint16_t u2t_limit;
    uint16_t u2t_accel;
    uint16_t u2t_decel;

    /* Driver request from joystick */
    if (s2t_joy_speed > 0)
    {
        u1t_req_dir = MOVE_DIR_FORWARD;
        u2t_joy_abs = (uint16_t) s2t_joy_speed;
    }
    else if (s2t_joy_speed < 0)
    {
        u1t_req_dir = MOVE_DIR_REVERSE;
        u2t_joy_abs = (uint16_t) (0 - s2t_joy_speed);
    }
    else
    {
        u1t_req_dir = MOVE_DIR_NONE;
        u2t_joy_abs = 0U;
    }

    /* Car is stopped: engage the requested direction */
    if (gu2t_speed_x100 == 0U)
    {
        gu1t_move_dir = u1t_req_dir;
    }
    else
    {
        /* keep current direction while rolling */
    }

    u2t_target = 0U;
    u2t_accel = ACCEL_DRIVE_PER_TICK;
    u2t_decel = DECEL_COAST_PER_TICK;

    if (safety_brake_is_active() == 1U)
    {
        /* Emergency brake: strong but gradual stop */
        u2t_decel = DECEL_EMERGENCY_PER_TICK;
    }
    else if (gu1t_state == GEAR_STATE_PARKED_LOCK)
    {
        gu2t_speed_x100 = 0U;
    }
    else if ((u1t_req_dir != MOVE_DIR_NONE) && (u1t_req_dir != gu1t_move_dir))
    {
        /* Opposite direction requested while rolling: brake to 0 first */
        u2t_decel = DECEL_BRAKE_PER_TICK;
    }
    else if (gu1t_move_dir == MOVE_DIR_FORWARD)
    {
        /* D: joystick 0..200 -> target 0..200 km/h, limited by gap / rain */
        u2t_limit = fsm_get_gap_limit();
        u2t_target = u2t_joy_abs;
        u2t_accel = ACCEL_DRIVE_PER_TICK;
        if (u2t_target > u2t_limit)
        {
            u2t_target = u2t_limit;
            u2t_decel = DECEL_BRAKE_PER_TICK;
        }
        else
        {
            u2t_decel = DECEL_COAST_PER_TICK;
        }
    }
    else if (gu1t_move_dir == MOVE_DIR_REVERSE)
    {
        /* R: joystick 0..200 -> target 0..SPEED_MAX_REVERSE km/h */
        u2t_limit = fsm_get_gap_limit();
        if (u2t_limit > SPEED_MAX_REVERSE)
        {
            u2t_limit = SPEED_MAX_REVERSE;
        }
        else
        {
            /* gap limit is stricter */
        }
        u2t_target = (uint16_t) ((u2t_joy_abs * SPEED_MAX_REVERSE) / SPEED_MAX_NORMAL);
        u2t_accel = ACCEL_REVERSE_PER_TICK;
        if (u2t_target > u2t_limit)
        {
            u2t_target = u2t_limit;
            u2t_decel = DECEL_BRAKE_PER_TICK;
        }
        else
        {
            u2t_decel = DECEL_COAST_PER_TICK;
        }
    }
    else
    {
        /* joystick released: coast down */
        u2t_decel = DECEL_COAST_PER_TICK;
    }

    /* Obstacle inside critical zone: automatic emergency braking */
    if ((fsm_is_critical_zone() == 1U) && (u2t_decel < DECEL_EMERGENCY_PER_TICK))
    {
        u2t_decel = DECEL_EMERGENCY_PER_TICK;
    }
    else
    {
        /* keep selected deceleration */
    }

    gu2t_speed_x100 = fsm_ramp(gu2t_speed_x100, (uint16_t) (u2t_target * SPEED_SCALE), u2t_accel, u2t_decel);
    gu2t_speed = gu2t_speed_x100 / SPEED_SCALE;

    if (gu2t_speed_x100 == 0U)
    {
        gu1t_move_dir = u1t_req_dir;
    }
    else
    {
        /* still rolling */
    }

    /* Gear shown follows actual motion (P-lock kept as is) */
    if (gu1t_state != GEAR_STATE_PARKED_LOCK)
    {
        if ((safety_brake_is_active() == 1U) && (gu2t_speed_x100 == 0U))
        {
            gu1t_state = GEAR_STATE_PARK;
        }
        else if (gu1t_move_dir == MOVE_DIR_FORWARD)
        {
            gu1t_state = GEAR_STATE_DRIVE;
        }
        else if (gu1t_move_dir == MOVE_DIR_REVERSE)
        {
            gu1t_state = GEAR_STATE_REVERSE;
        }
        else
        {
            gu1t_state = GEAR_STATE_PARK;
        }
    }
    else
    {
        /* parked lock */
    }
}

void gear_fsm_tick(void)
{
    uint16_t u2t_pot_raw;
    uint16_t u2t_joy_raw;
    int16_t s2t_joy_speed;

    /* Button events (EXTI edge + debounce in driver), handled here */
    fsm_handle_buttons(exti_buttons_scan_tick());

    u2t_pot_raw = adc_get_pot_raw();
    gu2t_gap_pct = sensor_get_closeness_pct(u2t_pot_raw);
    u2t_joy_raw = sensor_get_joy_axis_raw(adc_get_joy_vrx_raw(), adc_get_joy_vry_raw());
    s2t_joy_speed = sensor_get_joy_speed(adc_get_joy_vrx_raw(), adc_get_joy_vry_raw());

    fsm_update_speed(s2t_joy_speed);

    /* Warning LEDs by direction: PB6 for DRIVE, PA7 for REVERSE */
    if (gu1t_state == GEAR_STATE_DRIVE)
    {
        fsm_update_warning_led(gu2t_gap_pct, LED_ID_DRIVE_WARN);
        led_set(LED_ID_REVERSE_WARN, LED_STATE_OFF);
    }
    else if (gu1t_state == GEAR_STATE_REVERSE)
    {
        fsm_update_warning_led(gu2t_gap_pct, LED_ID_REVERSE_WARN);
        led_set(LED_ID_DRIVE_WARN, LED_STATE_OFF);
    }
    else
    {
        led_set(LED_ID_DRIVE_WARN, LED_STATE_OFF);
        led_set(LED_ID_REVERSE_WARN, LED_STATE_OFF);
    }

    /* Redlight buzzer logic (PA6 red LED mirrors buzzer) */
    fsm_update_redlight_buzzer(gu2t_gap_pct);

    /* Output: UART is queued to DMA, so OLED can run on the same tick */
    fsm_send_uart_dashboard(u2t_pot_raw, u2t_joy_raw);
    fsm_update_oled();
}

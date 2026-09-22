#include "driver_oled.h"
#include "board_config.h"
#include "driver_i2c.h"

#define OLED_CMD_PREFIX                   0x00U
#define OLED_DATA_PREFIX                  0x40U
#define OLED_ALT_I2C_ADDRESS              0x3DU
#define SH1106_COL_OFFSET                 2U
#define OLED_WIDTH                        128U
#define OLED_PAGES                        8U
#define OLED_PAGE_MASK                    0x07U
#define OLED_NIBBLE_MASK                  0x0FU
#define OLED_NIBBLE_SHIFT                 4U
#define OLED_STABILIZE_DELAY_COUNT        10000U
#define OLED_CMD_BURST_MAX                15U
#define OLED_CURSOR_CMD_COUNT             3U
#define FONT_WIDTH                        5U
#define FONT_CELL_WIDTH                   6U
#define FONT_FIRST_CHAR                   32U
#define FONT_LAST_CHAR                    126U
#define FONT_SPACE_INDEX                  0U
#define OLED_LINE_CHARS                   (OLED_WIDTH / FONT_CELL_WIDTH)
#define OLED_CHAR_PACKET_SIZE             (FONT_CELL_WIDTH + 1U)

/* SH1106 command set */
#define SH1106_CMD_DISPLAY_OFF            0xAEU
#define SH1106_CMD_DISPLAY_ON             0xAFU
#define SH1106_CMD_CLOCK_DIV              0xD5U
#define SH1106_VAL_CLOCK_DIV              0x80U
#define SH1106_CMD_MUX_RATIO              0xA8U
#define SH1106_VAL_MUX_64                 0x3FU
#define SH1106_CMD_DISPLAY_OFFSET         0xD3U
#define SH1106_VAL_OFFSET_ZERO            0x00U
#define SH1106_CMD_START_LINE_0           0x40U
#define SH1106_CMD_SEG_REMAP              0xA1U
#define SH1106_CMD_COM_SCAN_DEC           0xC8U
#define SH1106_CMD_COM_PINS               0xDAU
#define SH1106_VAL_COM_PINS_ALT           0x12U
#define SH1106_CMD_CONTRAST               0x81U
#define SH1106_VAL_CONTRAST               0x7FU
#define SH1106_CMD_RESUME_RAM             0xA4U
#define SH1106_CMD_NORMAL_DISPLAY         0xA6U
#define SH1106_CMD_PRECHARGE              0xD9U
#define SH1106_VAL_PRECHARGE              0xF1U
#define SH1106_CMD_VCOMH                  0xDBU
#define SH1106_VAL_VCOMH                  0x40U
#define SH1106_CMD_CHARGE_PUMP            0x8DU
#define SH1106_VAL_CHARGE_PUMP_ON         0x14U
#define SH1106_CMD_PAGE_ADDR              0xB0U
#define SH1106_CMD_COL_LOW                0x00U
#define SH1106_CMD_COL_HIGH               0x10U

static uint8_t gu1t_oled_detected = 0U;
static uint8_t gu1t_oled_addr = BOARD_OLED_I2C_ADDRESS;
static uint8_t gau1t_line_buf[OLED_WIDTH + 1U];

static uint8_t const gau1t_init_cmds[] = {
    SH1106_CMD_DISPLAY_OFF,
    SH1106_CMD_CLOCK_DIV, SH1106_VAL_CLOCK_DIV,
    SH1106_CMD_MUX_RATIO, SH1106_VAL_MUX_64,
    SH1106_CMD_DISPLAY_OFFSET, SH1106_VAL_OFFSET_ZERO,
    SH1106_CMD_START_LINE_0,
    SH1106_CMD_SEG_REMAP,
    SH1106_CMD_COM_SCAN_DEC,
    SH1106_CMD_COM_PINS, SH1106_VAL_COM_PINS_ALT,
    SH1106_CMD_CONTRAST, SH1106_VAL_CONTRAST,
    SH1106_CMD_RESUME_RAM,
    SH1106_CMD_NORMAL_DISPLAY,
    SH1106_CMD_PRECHARGE, SH1106_VAL_PRECHARGE,
    SH1106_CMD_VCOMH, SH1106_VAL_VCOMH,
    SH1106_CMD_CHARGE_PUMP, SH1106_VAL_CHARGE_PUMP_ON,
    SH1106_CMD_DISPLAY_ON
};

#define OLED_INIT_CMD_COUNT               ((uint16_t) sizeof(gau1t_init_cmds))

static uint8_t const gau1t_font5x7[][FONT_WIDTH] = {
    {0x00,0x00,0x00,0x00,0x00}, /* space */
    {0x00,0x00,0x5F,0x00,0x00}, /* ! */
    {0x00,0x07,0x00,0x07,0x00}, /* " */
    {0x14,0x7F,0x14,0x7F,0x14}, /* # */
    {0x24,0x2A,0x7F,0x2A,0x12}, /* $ */
    {0x23,0x13,0x08,0x64,0x62}, /* % */
    {0x36,0x49,0x55,0x22,0x50}, /* & */
    {0x00,0x05,0x03,0x00,0x00}, /* ' */
    {0x00,0x1C,0x22,0x41,0x00}, /* ( */
    {0x00,0x41,0x22,0x1C,0x00}, /* ) */
    {0x08,0x2A,0x1C,0x2A,0x08}, /* * */
    {0x08,0x08,0x3E,0x08,0x08}, /* + */
    {0x00,0x50,0x30,0x00,0x00}, /* , */
    {0x08,0x08,0x08,0x08,0x08}, /* - */
    {0x00,0x60,0x60,0x00,0x00}, /* . */
    {0x20,0x10,0x08,0x04,0x02}, /* / */
    {0x3E,0x51,0x49,0x45,0x3E}, /* 0 */
    {0x00,0x42,0x7F,0x40,0x00}, /* 1 */
    {0x42,0x61,0x51,0x49,0x46}, /* 2 */
    {0x21,0x41,0x45,0x4B,0x31}, /* 3 */
    {0x18,0x14,0x12,0x7F,0x10}, /* 4 */
    {0x27,0x45,0x45,0x45,0x39}, /* 5 */
    {0x3C,0x4A,0x49,0x49,0x30}, /* 6 */
    {0x01,0x71,0x09,0x05,0x03}, /* 7 */
    {0x36,0x49,0x49,0x49,0x36}, /* 8 */
    {0x06,0x49,0x49,0x29,0x1E}, /* 9 */
    {0x00,0x36,0x36,0x00,0x00}, /* : */
    {0x00,0x56,0x36,0x00,0x00}, /* ; */
    {0x00,0x08,0x14,0x22,0x41}, /* < */
    {0x14,0x14,0x14,0x14,0x14}, /* = */
    {0x41,0x22,0x14,0x08,0x00}, /* > */
    {0x02,0x01,0x51,0x09,0x06}, /* ? */
    {0x32,0x49,0x79,0x41,0x3E}, /* @ */
    {0x7E,0x11,0x11,0x11,0x7E}, /* A */
    {0x7F,0x49,0x49,0x49,0x36}, /* B */
    {0x3E,0x41,0x41,0x41,0x22}, /* C */
    {0x7F,0x41,0x41,0x22,0x1C}, /* D */
    {0x7F,0x49,0x49,0x49,0x41}, /* E */
    {0x7F,0x09,0x09,0x01,0x01}, /* F */
    {0x3E,0x41,0x41,0x51,0x32}, /* G */
    {0x7F,0x08,0x08,0x08,0x7F}, /* H */
    {0x00,0x41,0x7F,0x41,0x00}, /* I */
    {0x20,0x40,0x41,0x3F,0x01}, /* J */
    {0x7F,0x08,0x14,0x22,0x41}, /* K */
    {0x7F,0x40,0x40,0x40,0x40}, /* L */
    {0x7F,0x02,0x04,0x02,0x7F}, /* M */
    {0x7F,0x04,0x08,0x10,0x7F}, /* N */
    {0x3E,0x41,0x41,0x41,0x3E}, /* O */
    {0x7F,0x09,0x09,0x09,0x06}, /* P */
    {0x3E,0x41,0x51,0x21,0x5E}, /* Q */
    {0x7F,0x09,0x19,0x29,0x46}, /* R */
    {0x46,0x49,0x49,0x49,0x31}, /* S */
    {0x01,0x01,0x7F,0x01,0x01}, /* T */
    {0x3F,0x40,0x40,0x40,0x3F}, /* U */
    {0x1F,0x20,0x40,0x20,0x1F}, /* V */
    {0x7F,0x20,0x18,0x20,0x7F}, /* W */
    {0x63,0x14,0x08,0x14,0x63}, /* X */
    {0x03,0x04,0x78,0x04,0x03}, /* Y */
    {0x61,0x51,0x49,0x45,0x43}, /* Z */
    {0x00,0x00,0x7F,0x41,0x41}, /* [ */
    {0x02,0x04,0x08,0x10,0x20}, /* backslash */
    {0x41,0x41,0x7F,0x00,0x00}, /* ] */
    {0x04,0x02,0x01,0x02,0x04}, /* ^ */
    {0x40,0x40,0x40,0x40,0x40}, /* _ */
    {0x00,0x01,0x02,0x04,0x00}, /* ` */
    {0x20,0x54,0x54,0x54,0x78}, /* a */
    {0x7F,0x48,0x44,0x44,0x38}, /* b */
    {0x38,0x44,0x44,0x44,0x20}, /* c */
    {0x38,0x44,0x44,0x48,0x7F}, /* d */
    {0x38,0x54,0x54,0x54,0x18}, /* e */
    {0x08,0x7E,0x09,0x01,0x02}, /* f */
    {0x08,0x14,0x54,0x54,0x3C}, /* g */
    {0x7F,0x08,0x04,0x04,0x78}, /* h */
    {0x00,0x44,0x7D,0x40,0x00}, /* i */
    {0x20,0x40,0x44,0x3D,0x00}, /* j */
    {0x00,0x7F,0x10,0x28,0x44}, /* k */
    {0x00,0x41,0x7F,0x40,0x00}, /* l */
    {0x7C,0x04,0x18,0x04,0x78}, /* m */
    {0x7C,0x08,0x04,0x04,0x78}, /* n */
    {0x38,0x44,0x44,0x44,0x38}, /* o */
    {0x7C,0x14,0x14,0x14,0x08}, /* p */
    {0x08,0x14,0x14,0x18,0x7C}, /* q */
    {0x7C,0x08,0x04,0x04,0x08}, /* r */
    {0x48,0x54,0x54,0x54,0x20}, /* s */
    {0x04,0x3F,0x44,0x40,0x20}, /* t */
    {0x3C,0x40,0x40,0x20,0x7C}, /* u */
    {0x1C,0x20,0x40,0x20,0x1C}, /* v */
    {0x3C,0x40,0x30,0x40,0x3C}, /* w */
    {0x44,0x28,0x10,0x28,0x44}, /* x */
    {0x0C,0x50,0x50,0x50,0x3C}, /* y */
    {0x44,0x64,0x54,0x4C,0x44}, /* z */
    {0x00,0x08,0x36,0x41,0x00}, /* { */
    {0x00,0x00,0x7F,0x00,0x00}, /* | */
    {0x00,0x41,0x36,0x08,0x00}, /* } */
    {0x08,0x08,0x2A,0x1C,0x08}, /* ~ */
};

static void oled_send_cmd(uint8_t const u1t_cmd)
{
    uint8_t au1t_buf[2];

    au1t_buf[0] = OLED_CMD_PREFIX;
    au1t_buf[1] = u1t_cmd;
    (void) i2c1_write_bytes(gu1t_oled_addr, au1t_buf, 2U);
}

static void oled_send_cmd_burst(uint8_t const * const p_cmds, uint16_t const u2t_len)
{
    uint8_t au1t_buf[OLED_CMD_BURST_MAX + 1U];
    uint16_t u2t_idx;

    if (u2t_len <= OLED_CMD_BURST_MAX)
    {
        au1t_buf[0] = OLED_CMD_PREFIX;
        for (u2t_idx = 0U; u2t_idx < u2t_len; u2t_idx++)
        {
            au1t_buf[u2t_idx + 1U] = p_cmds[u2t_idx];
        }
        (void) i2c1_write_bytes(gu1t_oled_addr, au1t_buf, (uint16_t) (u2t_len + 1U));
    }
    else
    {
        /* burst too long, ignored */
    }
}

static uint8_t oled_get_font_index(char const c)
{
    uint8_t u1t_index;

    if ((c >= (char) FONT_FIRST_CHAR) && (c <= (char) FONT_LAST_CHAR))
    {
        u1t_index = (uint8_t) ((uint8_t) c - FONT_FIRST_CHAR);
    }
    else
    {
        u1t_index = FONT_SPACE_INDEX;
    }

    return u1t_index;
}

uint8_t oled_is_detected(void)
{
    return gu1t_oled_detected;
}

void oled_init(void)
{
    volatile uint32_t u4t_delay;
    uint16_t u2t_idx;

    gu1t_oled_detected = 0U;

    if (i2c1_probe(BOARD_OLED_I2C_ADDRESS) == 1U)
    {
        gu1t_oled_detected = 1U;
        gu1t_oled_addr = BOARD_OLED_I2C_ADDRESS;
    }
    else if (i2c1_probe(OLED_ALT_I2C_ADDRESS) == 1U)
    {
        gu1t_oled_detected = 1U;
        gu1t_oled_addr = OLED_ALT_I2C_ADDRESS;
    }
    else
    {
        /* OLED not found on bus - running in headless mode */
        gu1t_oled_detected = 0U;
    }

    if (gu1t_oled_detected == 1U)
    {
        for (u4t_delay = 0U; u4t_delay < OLED_STABILIZE_DELAY_COUNT; u4t_delay++)
        {
            /* power stabilization delay */
        }

        for (u2t_idx = 0U; u2t_idx < OLED_INIT_CMD_COUNT; u2t_idx++)
        {
            oled_send_cmd(gau1t_init_cmds[u2t_idx]);
        }

        oled_clear();
    }
    else
    {
        /* headless */
    }
}

void oled_set_cursor(uint8_t const u1t_page, uint8_t const u1t_col)
{
    uint8_t u1t_col_shifted;
    uint8_t au1t_cmds[OLED_CURSOR_CMD_COUNT];

    if (gu1t_oled_detected == 1U)
    {
        u1t_col_shifted = (uint8_t) (u1t_col + SH1106_COL_OFFSET);
        au1t_cmds[0] = (uint8_t) (SH1106_CMD_PAGE_ADDR | (u1t_page & OLED_PAGE_MASK));
        au1t_cmds[1] = (uint8_t) (SH1106_CMD_COL_LOW | (u1t_col_shifted & OLED_NIBBLE_MASK));
        au1t_cmds[2] = (uint8_t) (SH1106_CMD_COL_HIGH | ((u1t_col_shifted >> OLED_NIBBLE_SHIFT) & OLED_NIBBLE_MASK));
        oled_send_cmd_burst(au1t_cmds, OLED_CURSOR_CMD_COUNT);
    }
    else
    {
        /* headless */
    }
}

void oled_clear(void)
{
    uint16_t u2t_idx;
    uint8_t u1t_page;

    if (gu1t_oled_detected == 1U)
    {
        gau1t_line_buf[0] = OLED_DATA_PREFIX;
        for (u2t_idx = 1U; u2t_idx <= OLED_WIDTH; u2t_idx++)
        {
            gau1t_line_buf[u2t_idx] = 0x00U;
        }

        for (u1t_page = 0U; u1t_page < OLED_PAGES; u1t_page++)
        {
            oled_set_cursor(u1t_page, 0U);
            (void) i2c1_write_bytes(gu1t_oled_addr, gau1t_line_buf, (uint16_t) (OLED_WIDTH + 1U));
        }
    }
    else
    {
        /* headless */
    }
}

void oled_write_char(char const c)
{
    uint8_t au1t_data[OLED_CHAR_PACKET_SIZE];
    uint8_t u1t_font_idx;
    uint8_t u1t_col;

    if (gu1t_oled_detected == 1U)
    {
        u1t_font_idx = oled_get_font_index(c);
        au1t_data[0] = OLED_DATA_PREFIX;
        for (u1t_col = 0U; u1t_col < FONT_WIDTH; u1t_col++)
        {
            au1t_data[u1t_col + 1U] = gau1t_font5x7[u1t_font_idx][u1t_col];
        }
        au1t_data[FONT_WIDTH + 1U] = 0x00U;
        (void) i2c1_write_bytes(gu1t_oled_addr, au1t_data, OLED_CHAR_PACKET_SIZE);
    }
    else
    {
        /* headless */
    }
}

void oled_write_string(char const * const p_str)
{
    uint16_t u2t_idx;

    if (gu1t_oled_detected == 1U)
    {
        u2t_idx = 0U;
        while (p_str[u2t_idx] != '\0')
        {
            oled_write_char(p_str[u2t_idx]);
            u2t_idx++;
        }
    }
    else
    {
        /* headless */
    }
}

void oled_write_line(uint8_t const u1t_page, char const * const p_str)
{
    uint16_t u2t_char;
    uint16_t u2t_out;
    uint8_t u1t_col;
    uint8_t u1t_font_idx;
    uint8_t u1t_end;

    if (gu1t_oled_detected == 1U)
    {
        /* Render one full line (21 chars, padded with spaces) in ONE I2C transfer */
        gau1t_line_buf[0] = OLED_DATA_PREFIX;
        u2t_out = 1U;
        u1t_end = 0U;

        for (u2t_char = 0U; u2t_char < OLED_LINE_CHARS; u2t_char++)
        {
            if (u1t_end == 0U)
            {
                if (p_str[u2t_char] == '\0')
                {
                    u1t_end = 1U;
                }
                else
                {
                    /* keep reading */
                }
            }
            else
            {
                /* already padding */
            }

            if (u1t_end == 1U)
            {
                u1t_font_idx = FONT_SPACE_INDEX;
            }
            else
            {
                u1t_font_idx = oled_get_font_index(p_str[u2t_char]);
            }

            for (u1t_col = 0U; u1t_col < FONT_WIDTH; u1t_col++)
            {
                gau1t_line_buf[u2t_out] = gau1t_font5x7[u1t_font_idx][u1t_col];
                u2t_out++;
            }
            gau1t_line_buf[u2t_out] = 0x00U;
            u2t_out++;
        }

        oled_set_cursor(u1t_page, 0U);
        (void) i2c1_write_bytes(gu1t_oled_addr, gau1t_line_buf, u2t_out);
    }
    else
    {
        /* headless */
    }
}

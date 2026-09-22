#include "util_format.h"

#define DECIMAL_BASE                     10U
#define ASCII_ZERO_OFFSET                48U
#define DECIMAL_HUNDRED                  100U
#define DECIMAL_THOUSAND                 1000U

void util_format_u16_fixed3(uint16_t const u2t_value, char * const p_out)
{
    uint16_t u2t_hundreds;
    uint16_t u2t_tens;
    uint16_t u2t_ones;
    uint16_t u2t_remainder;

    u2t_hundreds = u2t_value / DECIMAL_HUNDRED;
    u2t_remainder = u2t_value % DECIMAL_HUNDRED;
    u2t_tens = u2t_remainder / DECIMAL_BASE;
    u2t_ones = u2t_remainder % DECIMAL_BASE;

    p_out[0] = (char) (ASCII_ZERO_OFFSET + u2t_hundreds);
    p_out[1] = (char) (ASCII_ZERO_OFFSET + u2t_tens);
    p_out[2] = (char) (ASCII_ZERO_OFFSET + u2t_ones);
}

uint16_t util_copy_str(char const * const p_src, char * const p_dest, uint16_t const u2t_dest_offset)
{
    uint16_t u2t_src_index;
    uint16_t u2t_dest_index;

    u2t_src_index = 0U;
    u2t_dest_index = u2t_dest_offset;

    while (p_src[u2t_src_index] != '\0')
    {
        p_dest[u2t_dest_index] = p_src[u2t_src_index];
        u2t_src_index++;
        u2t_dest_index++;
    }

    return u2t_dest_index;
}

void util_format_u16_fixed4(uint16_t const u2t_value, char * const p_out)
{
    uint16_t u2t_thousands;
    uint16_t u2t_hundreds;
    uint16_t u2t_tens;
    uint16_t u2t_ones;
    uint16_t u2t_remainder;

    u2t_thousands = u2t_value / DECIMAL_THOUSAND;
    u2t_remainder = u2t_value % DECIMAL_THOUSAND;
    u2t_hundreds = u2t_remainder / DECIMAL_HUNDRED;
    u2t_remainder = u2t_remainder % DECIMAL_HUNDRED;
    u2t_tens = u2t_remainder / DECIMAL_BASE;
    u2t_ones = u2t_remainder % DECIMAL_BASE;

    p_out[0] = (char) (ASCII_ZERO_OFFSET + u2t_thousands);
    p_out[1] = (char) (ASCII_ZERO_OFFSET + u2t_hundreds);
    p_out[2] = (char) (ASCII_ZERO_OFFSET + u2t_tens);
    p_out[3] = (char) (ASCII_ZERO_OFFSET + u2t_ones);
}

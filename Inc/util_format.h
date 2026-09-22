#ifndef UTIL_FORMAT_H
#define UTIL_FORMAT_H
#include "stm32f411xe.h"
void util_format_u16_fixed3(uint16_t const u2t_value, char * const p_out);
void util_format_u16_fixed4(uint16_t const u2t_value, char * const p_out);
uint16_t util_copy_str(char const * const p_src, char * const p_dest, uint16_t const u2t_dest_offset);
#endif

#ifndef DEBUG_H
#define DEBUG_H

#include <stdint.h>

void debug_puts(const char *s);

void debug_print_hex8(uint8_t value);
void debug_print_hex16(uint16_t value);
void debug_print_hex32(uint32_t value);
void debug_print_int(int value);

#endif

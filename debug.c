#include "debug.h"
#include "samd11.h"

static inline int semihosting_call(int reason, void *arg)
{
    int value;
    __asm__ volatile (
        "mov r0, %1      \n"
        "mov r1, %2      \n"
        "bkpt 0xAB       \n"
        "mov %0, r0      \n"
        : "=r" (value)
        : "r" (reason), "r" (arg)
        : "r0", "r1", "memory"
    );
    return value;
}

void debug_puts(const char *s)
{
    semihosting_call(0x04, (void *)s);
}

void debug_print_hex8(uint8_t value)
{
    const char *hex = "0123456789ABCDEF";
    char buf[4];

    buf[0] = hex[(value >> 4) & 0x0F];
    buf[1] = hex[value & 0x0F];
    buf[2] = '\n';
    buf[3] = '\0';

    debug_puts(buf);
}

void debug_print_hex16(uint16_t value)
{
    const char *hex = "0123456789ABCDEF";
    char buf[6];

    buf[0] = hex[(value >> 12) & 0x0F];
    buf[1] = hex[(value >> 8)  & 0x0F];
    buf[2] = hex[(value >> 4)  & 0x0F];
    buf[3] = hex[value & 0x0F];
    buf[4] = '\n';
    buf[5] = '\0';

    debug_puts(buf);
}

void debug_print_hex32(uint32_t value)
{
    const char *hex = "0123456789ABCDEF";
    char buf[10];

    buf[0] = hex[(value >> 28) & 0xF];
    buf[1] = hex[(value >> 24) & 0xF];
    buf[2] = hex[(value >> 20) & 0xF];
    buf[3] = hex[(value >> 16) & 0xF];
    buf[4] = hex[(value >> 12) & 0xF];
    buf[5] = hex[(value >> 8)  & 0xF];
    buf[6] = hex[(value >> 4)  & 0xF];
    buf[7] = hex[value & 0xF];
    buf[8] = '\n';
    buf[9] = '\0';

    debug_puts(buf);
}

void debug_print_int(int value)
{
    char buf[16];
    int i = 15;
    buf[i--] = '\0';

    if (value == 0) {
        buf[i] = '0';
        debug_puts(&buf[i]);
        return;
    }

    int v = value;
    int neg = 0;
    if (v < 0) {
        neg = 1;
        v = -v;
    }

    while (v > 0 && i >= 0) {
        buf[i--] = '0' + (v % 10);
        v /= 10;
    }

    if (neg && i >= 0) {
        buf[i--] = '-';
    }

    debug_puts(&buf[i + 1]);
    debug_puts("\n");
}

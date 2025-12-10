#include <stdint.h>
#include "samd11.h"
#include "device_pins.h"
#include "hal_i2c.h"

// ---------- Semihosting helpers ----------

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

static void debug_puts(const char *s)
{
    // ARM semihosting: SYS_WRITE0
    semihosting_call(0x04, (void *)s);
}

static void debug_print_hex8(uint8_t value)
{
    const char *hex = "0123456789ABCDEF";
    char buf[4];

    buf[0] = hex[(value >> 4) & 0x0F];
    buf[1] = hex[value & 0x0F];
    buf[2] = '\n';
    buf[3] = '\0';

    debug_puts(buf);
}

static void debug_print_hex16(uint16_t value)
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

static void debug_print_int(int value)
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
}

static void delay(volatile uint32_t t)
{
    while (t--) {
        __asm__("nop");
    }
}

// ---------- System init & dumps ----------

static void sys_init(void)
{
    // Disable prescaler: OSC8M / 1 → 8 MHz CPU
    SYSCTRL->OSC8M.bit.PRESC = 0;
}

static void debug_dump_system(void)
{
    debug_puts("\n==== SYSTEM DUMP ====\n");

    debug_puts("PM->APBCMASK = 0x");
    debug_print_hex16(PM->APBCMASK.reg);

    debug_puts("GCLK->CLKCTRL = 0x");
    debug_print_hex16(GCLK->CLKCTRL.reg);

    uint8_t pincfg8 = PORT->Group[0].PINCFG[8].reg;
    uint8_t pincfg9 = PORT->Group[0].PINCFG[9].reg;
    uint8_t pmux4   = PORT->Group[0].PMUX[4].reg; // PA08/PA09

    debug_puts("PINCFG[8] = 0x");
    debug_print_hex8(pincfg8);
    debug_puts("PINCFG[9] = 0x");
    debug_print_hex8(pincfg9);
    debug_puts("PMUX[4]   = 0x");
    debug_print_hex8(pmux4);

    uint32_t dir = PORT->Group[0].DIR.reg;
    uint32_t out = PORT->Group[0].OUT.reg;
    uint32_t in  = PORT->Group[0].IN.reg;

    debug_puts("DIR  = 0x");
    debug_print_hex16(dir & 0xFFFF);
    debug_puts("OUT  = 0x");
    debug_print_hex16(out & 0xFFFF);
    debug_puts("IN   = 0x");
    debug_print_hex16(in & 0xFFFF);

    debug_puts("==== END SYSTEM DUMP ====\n");
}

static void debug_dump_gclk(void)
{
    uint16_t clk = GCLK->CLKCTRL.reg;

    debug_puts("\nGCLK->CLKCTRL = 0x");
    debug_print_hex16(clk);

    debug_puts("  ID   = ");
    debug_print_int(clk & 0x3F);
    debug_puts("\n  GEN  = ");
    debug_print_int((clk >> 8) & 0xF);
    debug_puts("\n  CLKEN= ");
    debug_print_int((clk >> 14) & 0x1);
    debug_puts("\n");
}

static void debug_dump_i2c(const char *tag)
{
    debug_puts("\n---- I2C DUMP: ");
    debug_puts(tag);
    debug_puts(" ----\n");

    uint16_t ctrla  = SMARTAG_I2C_SERCOM->I2CM.CTRLA.reg;
    uint16_t ctrlb  = SMARTAG_I2C_SERCOM->I2CM.CTRLB.reg;
    uint16_t baud   = SMARTAG_I2C_SERCOM->I2CM.BAUD.reg;
    uint16_t status = SMARTAG_I2C_SERCOM->I2CM.STATUS.reg;
    uint8_t  intfl  = SMARTAG_I2C_SERCOM->I2CM.INTFLAG.reg;
    uint8_t  sync   = SMARTAG_I2C_SERCOM->I2CM.SYNCBUSY.reg;
    uint16_t addr   = SMARTAG_I2C_SERCOM->I2CM.ADDR.reg;
    uint8_t  data   = SMARTAG_I2C_SERCOM->I2CM.DATA.reg;

    debug_puts("CTRLA = 0x");
    debug_print_hex16(ctrla);
    debug_puts("  MODE    = ");
    debug_print_int(SMARTAG_I2C_SERCOM->I2CM.CTRLA.bit.MODE);
    debug_puts("\n  SPEED   = ");
    debug_print_int(SMARTAG_I2C_SERCOM->I2CM.CTRLA.bit.SPEED);
    debug_puts("\n  SDAHOLD = ");
    debug_print_int(SMARTAG_I2C_SERCOM->I2CM.CTRLA.bit.SDAHOLD);
    debug_puts("\n  PINOUT  = ");
    debug_print_int(SMARTAG_I2C_SERCOM->I2CM.CTRLA.bit.PINOUT);
    debug_puts("\n");

    debug_puts("CTRLB = 0x");
    debug_print_hex16(ctrlb);
    debug_puts("  SMEN    = ");
    debug_print_int(SMARTAG_I2C_SERCOM->I2CM.CTRLB.bit.SMEN);
    debug_puts("\n  ACKACT  = ");
    debug_print_int(SMARTAG_I2C_SERCOM->I2CM.CTRLB.bit.ACKACT);
    debug_puts("\n  CMD     = ");
    debug_print_int(SMARTAG_I2C_SERCOM->I2CM.CTRLB.bit.CMD);
    debug_puts("\n");

    debug_puts("BAUD  = 0x");
    debug_print_hex16(baud);

    debug_puts("STATUS = 0x");
    debug_print_hex16(status);
    debug_puts("  BUSSTATE = ");
    debug_print_int(SMARTAG_I2C_SERCOM->I2CM.STATUS.bit.BUSSTATE);
    debug_puts("\n  BUSERR   = ");
    debug_print_int(SMARTAG_I2C_SERCOM->I2CM.STATUS.bit.BUSERR);
    debug_puts("\n  ARBLOST  = ");
    debug_print_int(SMARTAG_I2C_SERCOM->I2CM.STATUS.bit.ARBLOST);
    debug_puts("\n  RXNACK   = ");
    debug_print_int(SMARTAG_I2C_SERCOM->I2CM.STATUS.bit.RXNACK);
    debug_puts("\n");

    debug_puts("INTFLAG = 0x");
    debug_print_hex8(intfl);
    debug_puts("  MB      = ");
    debug_print_int(SMARTAG_I2C_SERCOM->I2CM.INTFLAG.bit.MB);
    debug_puts("\n  SB      = ");
    debug_print_int(SMARTAG_I2C_SERCOM->I2CM.INTFLAG.bit.SB);
    debug_puts("\n  ERROR   = ");
    debug_print_int(SMARTAG_I2C_SERCOM->I2CM.INTFLAG.bit.ERROR);
    debug_puts("\n");

    debug_puts("SYNCBUSY = 0x");
    debug_print_hex8(sync);

    debug_puts("ADDR = 0x");
    debug_print_hex16(addr);
    debug_puts("DATA = 0x");
    debug_print_hex8(data);
}

int main(void)
{
    sys_init();

    debug_puts("\n*** SAMD11 I2C bring-up ***\n");

    debug_dump_system();

    debug_dump_i2c("Before init");
    hal_i2c_init();
    debug_dump_i2c("After init");
    debug_dump_gclk();

    while (1) {
        debug_puts("\nI2C State BEFORE write\n");
        debug_dump_i2c("Before write");

        hal_i2c_test_write(0x01, 0xFF);

        debug_puts("[I2C] State AFTER write\n");
        debug_dump_i2c("After write");

        delay(100000);
    }

    return 0;
}
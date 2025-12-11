#include <stdint.h>
#include "samd11.h"
#include "debug.h"
#include "hal_i2c.h"

static void sys_init(void)
{
    SYSCTRL->OSC8M.bit.PRESC   = 0;
    SYSCTRL->OSC8M.bit.ONDEMAND = 0;
    SYSCTRL->OSC8M.bit.ENABLE  = 1;
    while (!SYSCTRL->PCLKSR.bit.OSC8MRDY);

    GCLK->GENCTRL.reg =
          GCLK_GENCTRL_ID(0)
        | GCLK_GENCTRL_SRC_OSC8M
        | GCLK_GENCTRL_GENEN;
    while (GCLK->STATUS.bit.SYNCBUSY);
}


static void gpio_test_pa04_pa07(void)
{
    // 1) Disable SERCOM0 I²C
    SERCOM0->I2CM.CTRLA.bit.ENABLE = 0;
    while (SERCOM0->I2CM.SYNCBUSY.bit.ENABLE);

    // 2) Disconnect peripheral mux
    PORT->Group[0].PINCFG[4].bit.PMUXEN = 0;  // PA04
    PORT->Group[0].PINCFG[7].bit.PMUXEN = 0;  // PA07

    // 3) Make them outputs
    PORT->Group[0].DIRSET.reg = (1u << 4) | (1u << 7);

    while (1) {
        // both high
        PORT->Group[0].OUTSET.reg = (1u << 4) | (1u << 7);
        for (volatile uint32_t i = 0; i < 50000; i++);

        // both low
        PORT->Group[0].OUTCLR.reg = (1u << 4) | (1u << 7);
        for (volatile uint32_t i = 0; i < 50000; i++);
    }
}


int main(void)
{
    sys_init();

    //gpio_test_pa04_pa07();

    debug_puts("Dump bevor I2C Init");
    hal_i2c_dump_regs();

    debug_puts("\n\nDump after I2C Init");
    hal_i2c_init();
    hal_i2c_dump_regs(); 

    while (1) {
        hal_i2c_test_write();
    }
}

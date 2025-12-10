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

int main(void)
{
    sys_init();

    debug_puts("Dump bevor I2C Init");
    hal_i2c_dump_regs();

    debug_puts("\n\nDump after I2C Init");
    hal_i2c_init();
    hal_i2c_dump_regs(); 


    while (1) {
        hal_i2c_test_write();
    }
}

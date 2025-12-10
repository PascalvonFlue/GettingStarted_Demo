#include "hal_i2c.h"
#include "device_pins.h"
#include "debug.h"

static inline void pin_set_peripheral_function(uint32_t pinmux){
    uint8_t port = (uint8_t)((pinmux >> 16)/32);
    PORT->Group[port].PINCFG[((pinmux >> 16) - (port*32))].bit.PMUXEN = 1;
    PORT->Group[port].PMUX[((pinmux >> 16) - (port*32))/2].reg &= ~(0xF << (4 * ((pinmux >>
    16) & 0x01u)));
    PORT->Group[port].PMUX[((pinmux >> 16) - (port*32))/2].reg |= (uint8_t)((pinmux &
    0x0000FFFF) << (4 * ((pinmux >> 16) & 0x01u)));
}

void i2c_clock_init(void)
{
    // Turn on GCLK module on APBA
    PM->APBAMASK.reg |= PM_APBAMASK_GCLK;

    GCLK->CTRL.reg = GCLK_CTRL_SWRST;
    while (GCLK->CTRL.reg & GCLK_CTRL_SWRST) {}

    // Generator 0: source = OSC8M, enabled
    GCLK->GENCTRL.reg =
          GCLK_GENCTRL_ID(0)
        | GCLK_GENCTRL_SRC_OSC8M
        | GCLK_GENCTRL_GENEN;
    while (GCLK->STATUS.bit.SYNCBUSY) {}

    // Turn on SERCOM0 in PM APBC
    PM->APBCMASK.reg |= PM_APBCMASK_SERCOM0;

    // Route GCLK0 to SERCOM0 core
    *((volatile uint8_t*)&GCLK->CLKCTRL.reg) = SMARTAG_I2C_SERCOM_GCLK_ID_CORE;
    while (GCLK->STATUS.bit.SYNCBUSY) {}
    GCLK->CLKCTRL.reg =
          GCLK_CLKCTRL_GEN(0)
        | GCLK_CLKCTRL_ID(SMARTAG_I2C_SERCOM_GCLK_ID_CORE)
        | GCLK_CLKCTRL_CLKEN;
    while (GCLK->STATUS.bit.SYNCBUSY) {}

    // Route GCLK0 to SERCOM0 slow
    *((volatile uint8_t*)&GCLK->CLKCTRL.reg) = SMARTAG_I2C_SERCOM_GCLK_ID_SLOW;
    while (GCLK->STATUS.bit.SYNCBUSY) {}
    GCLK->CLKCTRL.reg =
          GCLK_CLKCTRL_GEN(0)
        | GCLK_CLKCTRL_ID(SMARTAG_I2C_SERCOM_GCLK_ID_SLOW)
        | GCLK_CLKCTRL_CLKEN;
    while (GCLK->STATUS.bit.SYNCBUSY) {}
}

void i2c_pin_init(void)
{
    // PA08 - Pin4 - SDA and PA09 - PIN 5 - SCL
    pin_set_peripheral_function(SMARTAG_I2C_SDA_PINMUX);
    pin_set_peripheral_function(SMARTAG_I2C_SCL_PINMUX);
}

void hal_i2c_init(void){

    i2c_clock_init();

    i2c_pin_init();

    SERCOM0->I2CM.CTRLA.bit.ENABLE = 0;         //Disable befor Mod
    while (SERCOM0->I2CM.SYNCBUSY.bit.ENABLE);

    SERCOM0->I2CM.CTRLA.bit.SWRST = 1;          // Software reset
    while (SERCOM0->I2CM.SYNCBUSY.bit.SWRST);

    SERCOM0->I2CM.CTRLA.bit.SPEED = 0;          //Standar speed
    SERCOM0->I2CM.CTRLA.bit.SDAHOLD = 0x2;      //Hold time 300 - 600ns
    SERCOM0->I2CM.CTRLA.bit.RUNSTDBY = 1;       //Generic clock enabled in all sleep modes
    SERCOM0->I2CM.CTRLA.bit.MODE = 0x5;         //Master mode
    SERCOM0->I2CM.CTRLA.bit.PINOUT = 0;         // 2 Wire
    SERCOM0->I2CM.CTRLB.bit.SMEN = 1;           //Smart mode
    while(SERCOM0->I2CM.SYNCBUSY.bit.SYSOP);    //Sync

    SERCOM0->I2CM.BAUD.bit.BAUD = 0x34;         //100k on 8MHz
    SERCOM0->I2CM.BAUD.bit.BAUDLOW = 0x34;
    while(SERCOM0->I2CM.SYNCBUSY.bit.SYSOP);    //Sync

    SERCOM0->I2CM.CTRLA.bit.ENABLE = 1;         // Enable bus
    while ((SERCOM0->I2CM.SYNCBUSY.bit.SYSOP & SERCOM0->I2CM.SYNCBUSY.bit.ENABLE));

    SERCOM0->I2CM.STATUS.bit.BUSSTATE = 1;      // Set to idel
    while (SERCOM0->I2CM.SYNCBUSY.bit.SYSOP);
}

static void debug_dump_gclk_channel(uint8_t id, const char *name){
    debug_puts(name);
    debug_puts(" CLKCTRL = ");

    // Select this channel
    *((volatile uint8_t*)&GCLK->CLKCTRL.reg) = id;
    while (GCLK->STATUS.bit.SYNCBUSY) {}

    debug_print_hex16(GCLK->CLKCTRL.reg);
}

void hal_i2c_test_write(void){
    uint16_t SLAVE_ADDR = 0x01;
    SERCOM0->I2CM.CTRLB.reg &= ~SERCOM_I2CM_CTRLB_ACKACT; //writing 0 in ACKACT bit
    while(SERCOM0->I2CM.SYNCBUSY.bit.SYSOP);
    SERCOM0->I2CM.ADDR.reg = (SLAVE_ADDR << 1) | 0;
}

void hal_i2c_dump_regs(void){
    debug_puts("==== I2C DUMP ====\n");

    debug_puts("CTRLA = ");
    debug_print_hex32(SMARTAG_I2C_SERCOM->I2CM.CTRLA.reg);

    debug_puts("CTRLB   = ");
    debug_print_hex32(SMARTAG_I2C_SERCOM->I2CM.CTRLB.reg);

    debug_puts("BAUD    = ");
    debug_print_hex16(SMARTAG_I2C_SERCOM->I2CM.BAUD.reg);

    debug_puts("STATUS  = ");
    debug_print_hex16(SMARTAG_I2C_SERCOM->I2CM.STATUS.reg);

    debug_puts("INTFLAG = ");
    debug_print_hex8(SMARTAG_I2C_SERCOM->I2CM.INTFLAG.reg);

    debug_puts("BUSSTATE= ");
    debug_print_int(SMARTAG_I2C_SERCOM->I2CM.STATUS.bit.BUSSTATE);

    debug_puts("RXNACK  = ");
    debug_print_int(SMARTAG_I2C_SERCOM->I2CM.STATUS.bit.RXNACK);

    debug_puts("ERROR   = ");
    debug_print_int(SMARTAG_I2C_SERCOM->I2CM.INTFLAG.bit.ERROR);

    debug_puts(" PM->APBCMASK = ");
    debug_print_hex16(PM->APBCMASK.reg);

    debug_dump_gclk_channel(SMARTAG_I2C_SERCOM_GCLK_ID_CORE, "SERCOM0_CORE");
    debug_dump_gclk_channel(SMARTAG_I2C_SERCOM_GCLK_ID_SLOW, "SERCOM0_SLOW");

    debug_puts("==== END I2C DUMP ====\n");
}
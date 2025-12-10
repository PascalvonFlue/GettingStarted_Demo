#include "hal_i2c.h"
#include "device_pins.h"

static void i2c_enable_gclk(void)
{
    //Enable APB clock to SERCOM0
    PM->APBCMASK.reg |= SMARTAG_I2C_SERCOM_APBCMASK_BIT;

    //Route GCLK0 to SERCOM0 core & slow clocks
    uint16_t clk_core =
          GCLK_CLKCTRL_CLKEN                
        | GCLK_CLKCTRL_GEN(0)
        | GCLK_CLKCTRL_ID(SMARTAG_I2C_SERCOM_GCLK_ID_CORE);

    GCLK->CLKCTRL.reg = clk_core;
    while (GCLK->STATUS.bit.SYNCBUSY);

    uint16_t clk_slow =
          GCLK_CLKCTRL_CLKEN
        | GCLK_CLKCTRL_GEN(0)
        | GCLK_CLKCTRL_ID(SMARTAG_I2C_SERCOM_GCLK_ID_SLOW);

    GCLK->CLKCTRL.reg = clk_slow;
    while (GCLK->STATUS.bit.SYNCBUSY);
}

void hal_i2c_init(void)
{
    i2c_enable_gclk();

    // Disabel for config
    SMARTAG_I2C_SERCOM->I2CM.CTRLA.bit.ENABLE = 0;
    while (SMARTAG_I2C_SERCOM->I2CM.SYNCBUSY.bit.ENABLE);

    const uint32_t sda_mask = (1u << SMARTAG_I2C_SDA_PIN_INDEX); // PA08
    const uint32_t scl_mask = (1u << SMARTAG_I2C_SCL_PIN_INDEX); // PA09

    PORT->Group[0].PINCFG[SMARTAG_I2C_SDA_PIN_INDEX].reg =
          PORT_PINCFG_PMUXEN
        | PORT_PINCFG_INEN;

    PORT->Group[0].PINCFG[SMARTAG_I2C_SCL_PIN_INDEX].reg =
          PORT_PINCFG_PMUXEN
        | PORT_PINCFG_INEN;

    PORT->Group[0].OUTSET.reg = sda_mask | scl_mask;

    uint8_t pmux_val = 0;
    pmux_val |= PORT_PMUX_PMUXE(SMARTAG_I2C_SDA_PMUX_VALUE);
    pmux_val |= PORT_PMUX_PMUXO(SMARTAG_I2C_SCL_PMUX_VALUE);
    PORT->Group[0].PMUX[SMARTAG_I2C_SDA_PIN_INDEX / 2].reg = pmux_val;

    SMARTAG_I2C_SERCOM->I2CM.CTRLA.reg = SERCOM_I2CM_CTRLA_SWRST;
    while (SMARTAG_I2C_SERCOM->I2CM.SYNCBUSY.bit.SWRST);
    while (SMARTAG_I2C_SERCOM->I2CM.CTRLA.bit.SWRST ||
           SMARTAG_I2C_SERCOM->I2CM.CTRLA.bit.ENABLE);

    uint32_t ctrla = 0;

    ctrla |= SERCOM_I2CM_CTRLA_MODE_I2C_MASTER; // I2C master
    ctrla |= SERCOM_I2CM_CTRLA_SPEED(0x0);      // Standard Speed
    ctrla |= SERCOM_I2CM_CTRLA_SDAHOLD(0x2);    // SDA hold time = 2
    ctrla |= SERCOM_I2CM_CTRLA_PINOUT;          // Use SDA/SCL on PAD[2:3]

    SMARTAG_I2C_SERCOM->I2CM.CTRLA.reg = ctrla;

    uint32_t ctrlb = 0;
    ctrlb |= SERCOM_I2CM_CTRLB_SMEN;
    SMARTAG_I2C_SERCOM->I2CM.CTRLB.reg = ctrlb;

    SMARTAG_I2C_SERCOM->I2CM.BAUD.reg = 35; //100kHz

    SMARTAG_I2C_SERCOM->I2CM.CTRLA.bit.ENABLE = 1;
    while (SMARTAG_I2C_SERCOM->I2CM.SYNCBUSY.bit.ENABLE);

    //Force bus IDLE
    SMARTAG_I2C_SERCOM->I2CM.STATUS.reg = SERCOM_I2CM_STATUS_BUSSTATE(1);
    while (SMARTAG_I2C_SERCOM->I2CM.SYNCBUSY.bit.SYSOP);
}

bool hal_i2c_test_write(uint8_t addr7, uint8_t data)
{
    uint8_t addr_byte = (addr7 << 1);

    SMARTAG_I2C_SERCOM->I2CM.STATUS.reg =
          SERCOM_I2CM_STATUS_BUSERR
        | SERCOM_I2CM_STATUS_ARBLOST
        | SERCOM_I2CM_STATUS_RXNACK;

    SMARTAG_I2C_SERCOM->I2CM.INTFLAG.reg =
          SERCOM_I2CM_INTFLAG_MB
        | SERCOM_I2CM_INTFLAG_SB
        | SERCOM_I2CM_INTFLAG_ERROR;

    SMARTAG_I2C_SERCOM->I2CM.ADDR.reg = addr_byte;

    uint32_t timeout = 100000;
    while (!(SMARTAG_I2C_SERCOM->I2CM.INTFLAG.reg &
             (SERCOM_I2CM_INTFLAG_MB | SERCOM_I2CM_INTFLAG_ERROR))) {
        if (--timeout == 0) {
            return false;
        }
    }

    SMARTAG_I2C_SERCOM->I2CM.CTRLB.reg = SERCOM_I2CM_CTRLB_CMD(3); // STOP

    return true;
}
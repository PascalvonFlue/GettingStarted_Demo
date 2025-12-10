Current Issue

I’m working on an I²C master driver on an ATSAMD11 using SERCOM0.
I’ve written a small debug function that dumps the SERCOM I²C register state and the relevant generic clock (GCLK) settings before and after my hal_i2c_init() function.

Dump before I²C init:
Dump bevor I2C Init
==== I2C DUMP ====
CTRLA = 00000000
CTRLB   = 00000000
BAUD    = 0000
STATUS  = 0000
INTFLAG = 00
BUSSTATE= 0RXNACK  = 0ERROR   = 0 PM->APBCMASK = 0100
SERCOM0_CORE CLKCTRL = 000E
SERCOM0_SLOW CLKCTRL = 000D
==== END I2C DUMP ====

Dump after hal_i2c_init():
Dump after I2C Init
==== I2C DUMP ====
CTRLA = 00200096
CTRLB   = 00000100
BAUD    = 3434
STATUS  = 0010
INTFLAG = 00
BUSSTATE= 1
RXNACK  = 0ERROR   = 0 PM->APBCMASK = 0104
SERCOM0_CORE CLKCTRL = 400E
SERCOM0_SLOW CLKCTRL = 000D
==== END I2C DUMP ====


My concerns:

SERCOM and BAUD look correct after init:
- CTRLA = 0x00200096 → SERCOM0 is in I²C master mode, enabled, SDAHOLD=2, RUNSTDBY set.
- CTRLB = 0x00000100 → SMEN (smart mode) enabled.
- BAUD = 0x3434 → BAUD=0x34, BAUDLOW=0x34, which should be ~100 kHz at 8 MHz.
- STATUS = 0x0010, BUSSTATE = 1 → bus forced to IDLE, no errors.
- GCLK for the SERCOM0 core looks correct:
- SERCOM0_CORE CLKCTRL = 0x400E

So I would expect SERCOM0_SLOW CLKCTRL to show 0x400D after init (same CLKEN bit set as the core clock), but it remains 0x000D.

Because I currently don’t see any toggling on SDA/SCL (I’m just trying to see the lines wiggle at all), I’m worried that this “slow clock not enabled” is the reason, even though the core clock is clearly enabled and SERCOM0 is configured as an I²C master.
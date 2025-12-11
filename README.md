ATSAMD11 SERCOM0 I²C Debug Notes


Initial Problem
I was configuring SERCOM0 as an I²C master and noticed that the bus lines never toggled (SDA remained low, SCL stayed high or low depending on state).
Before and after calling my hal_i2c_init(), I dumped the complete SERCOM and GCLK registers to confirm the clocks were configured correctly.

Dump Before I²C Init
==== I2C DUMP ====
CTRLA    = 00000000
CTRLB    = 00000000
BAUD     = 0000
STATUS   = 0000
INTFLAG  = 00
BUSSTATE = 0
RXNACK   = 0
ERROR    = 0
PM->APBCMASK = 0100
SERCOM0_CORE CLKCTRL = 000E
SERCOM0_SLOW CLKCTRL = 000D
==== END I2C DUMP ====

Dump After hal_i2c_init()
==== I2C DUMP ====
CTRLA    = 00200096
CTRLB    = 00000100
BAUD     = 3434
STATUS   = 0010
INTFLAG  = 00
BUSSTATE = 1
RXNACK   = 0
ERROR    = 0
PM->APBCMASK = 0104
SERCOM0_CORE CLKCTRL = 400E
SERCOM0_SLOW CLKCTRL = 000D   <── suspicious
==== END I2C DUMP ====

My concern:
SERCOM0_SLOW never showed the CLKEN bit (0x4000), even after init.
Given that the I²C timing engine uses the slow clock for timeouts, I assumed this was the reason SCL/SDA never toggled.
Turns out that assumption was only partially correct.

Root Causes
1. Incorrect pin selection: PA08/PA09 cannot be used for 2-wire I²C

Even though PA08 and PA09 support SERCOM0_PAD2 and SERCOM0_PAD3,
I²C master mode on SAMD11 only works on:

PAD0 → SDA
PAD1 → SCL


Fixed by switching to valid pads:
#define SMARTAG_I2C_SDA_PINMUX  PINMUX_PA04D_SERCOM0_PAD0
#define SMARTAG_I2C_SCL_PINMUX  PINMUX_PA07C_SERCOM0_PAD1


After selecting PA04/PA07 and adding 10 kΩ pull-ups, the SERCOM finally generated SCL activity.

2. The I²C timeout engine requires the slow clock to be active

The SAMD11 I²C hardware includes SMBus-compatible timeout features.
These are driven by the GCLK_SERCOM_SLOW clock.

For master mode to operate correctly, the timeout logic must be enabled:

SMARTAG_I2C_SERCOM->I2CM.CTRLA.bit.LOWTOUTEN = 1;


Without this bit set:

The state machine may hold SCL low (CLKHOLD=1)

Transactions may never complete

No STOP condition is ever generated

SDA/SCL appear completely dead on the scope

After enabling LOWTOUTEN, the slow clock becomes active and the state machine behaves correctly.


Final Working Configuration

SDA = PA04 (PAD0)
SCL = PA07 (PAD1)
10 kΩ pull-ups on both lines

CTRLA.PINOUT = 0 (standard 2-wire)
CTRLA.LOWTOUTEN = 1 (enables slow-clock timeout engine)
Proper STOP command at end of transaction

Result

After fixing both issues:
SDA/SCL waveforms appear correctly
Address + data bytes clock out even without a slave
No stuck-low behavior
BAUD of 0x23 gives a clean 100 kHz clock at 8 MHz

The peripheral finally reports:

INTFLAG = MB
BUSSTATE = OWNER
RXNACK = 1   (expected without a slave)
#pragma once

#include <stdint.h>
#include <stdbool.h>

void hal_i2c_init(void);

void hal_i2c_dump_regs(void);

void hal_i2c_test_write(void);
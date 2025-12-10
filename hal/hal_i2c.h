#pragma once
#include <stdint.h>
#include <stdbool.h>

void hal_i2c_init(void);
bool hal_i2c_test_write(uint8_t addr7, uint8_t data);
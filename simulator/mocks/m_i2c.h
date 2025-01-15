#ifndef MOCK_I2C_H
#define MOCK_I2C_H

#define PLATFORM_I2C_CPP

#include "src/system/platform/i2c.h"

#include <cstdint>

void i2c_setup(uint8_t i2cIndex, uint32_t baudrate, uint32_t timeout) {}

int i2c_writeData(uint8_t i2cIndex, uint8_t deviceAddr, uint8_t registerAdd, uint8_t size, uint8_t* buf, int stopBit)
{
  return 0;
}

int i2c_readData(uint8_t i2cIndex, uint8_t deviceAddr, uint8_t registerAdd, uint8_t size, uint8_t* buf, int stopBit)
{
  return 0;
}

int i2c_xfer(
        uint8_t i2cIndex, uint8_t deviceAddr, int out_size, const uint8_t* out, int in_size, uint8_t* in, uint8_t flags)
{
  return 0;
}

#endif
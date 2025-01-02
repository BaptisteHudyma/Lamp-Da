#ifndef MOCK_I2C_H
#define MOCK_I2C_H

#define PLATFORM_I2C

#include <cstdint>

inline void i2c_setup(uint8_t i2cIndex, uint32_t baudrate, uint32_t timeout) {}

inline int i2c_writeData(
        uint8_t i2cIndex, uint8_t deviceAddr, uint8_t registerAdd, uint8_t size, uint8_t* buf, int stopBit)
{
  return 0;
}

inline int i2c_readData(
        uint8_t i2cIndex, uint8_t deviceAddr, uint8_t registerAdd, uint8_t size, uint8_t* buf, int stopBit)
{
  return 0;
}

inline int i2c_xfer(
        uint8_t i2cIndex, uint8_t deviceAddr, int out_size, const uint8_t* out, int in_size, uint8_t* in, uint8_t flags)
{
  return 0;
}

inline int i2c_read8(uint8_t i2cIndex, uint8_t deviceAddr, uint8_t registerAdd, uint8_t* val, int stopBit)
{
  return i2c_readData(i2cIndex, deviceAddr, registerAdd, 1, val, stopBit);
}

inline int i2c_write8(uint8_t i2cIndex, uint8_t deviceAddr, uint8_t registerAdd, uint8_t val, int stopBit)
{
  return i2c_writeData(i2cIndex, deviceAddr, registerAdd, 1, &val, stopBit);
}

inline int i2c_read16(uint8_t i2cIndex, uint8_t deviceAddr, uint8_t registerAdd, uint16_t* val, int stopBit)
{
  return i2c_readData(i2cIndex, deviceAddr, registerAdd, 2, (uint8_t*)val, stopBit);
}

inline int i2c_write16(uint8_t i2cIndex, uint8_t deviceAddr, uint8_t registerAdd, uint16_t val, int stopBit)
{
  uint8_t data[2];
  data[0] = val & 0xFF;
  data[1] = (val >> 8) & 0xFF;
  return i2c_writeData(i2cIndex, deviceAddr, registerAdd, 2, data, stopBit);
}

#endif
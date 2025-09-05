#include "src/system/platform/i2c.h"

#define PLATFORM_I2C_CPP

#include <cstdint>

void i2c_setup(uint8_t i2cIndex, uint32_t baudrate, uint32_t timeout) {}

int i2c_check_existence(uint8_t i2cIndex, uint8_t deviceAddr)
{
  // error status
  switch (i2cIndex)
  {
    case pdNegociationI2cAddress:
    case chargeI2cAddress:
    case imuI2cAddress:
    case batteryBalancerI2cAddress:
      {
        return 0;
      }
  }
  return 1;
}

int i2c_writeData(uint8_t i2cIndex, uint8_t deviceAddr, uint8_t registerAdd, uint8_t size, uint8_t* buf, int stopBit)
{
  switch (i2cIndex)
  {
    case pdNegociationI2cAddress:
      {
        break;
      }
    case chargeI2cAddress:
      {
        break;
      }
    case imuI2cAddress:
      {
        break;
      }
    case batteryBalancerI2cAddress:
      {
        break;
      }
  }
  return 1;
}

int i2c_readData(uint8_t i2cIndex, uint8_t deviceAddr, uint8_t registerAdd, uint8_t size, uint8_t* buf, int stopBit)
{
  switch (i2cIndex)
  {
    case pdNegociationI2cAddress:
      {
        break;
      }
    case chargeI2cAddress:
      {
        break;
      }
    case imuI2cAddress:
      {
        break;
      }
    case batteryBalancerI2cAddress:
      {
        break;
      }
  }
  return 1;
}

int i2c_xfer(
        uint8_t i2cIndex, uint8_t deviceAddr, int out_size, const uint8_t* out, int in_size, uint8_t* in, uint8_t flags)
{
  switch (i2cIndex)
  {
    case pdNegociationI2cAddress:
      {
        break;
      }
    case chargeI2cAddress:
      {
        break;
      }
    case imuI2cAddress:
      {
        break;
      }
    case batteryBalancerI2cAddress:
      {
        break;
      }
  }
  return 1;
}

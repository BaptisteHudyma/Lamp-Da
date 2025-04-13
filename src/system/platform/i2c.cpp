#ifndef PLATFORM_I2C_CPP
#define PLATFORM_I2C_CPP

#include "i2c.h"

#include "time.h"

#include <cassert>

// platform specific code
#include "Wire.h"
#include "rtos.h" // tied to FreeRTOS for serialization

// set the two interfaces
TwoWire* PROGMEM interfaces[] = {&Wire};

// mutex to prevent i2c lockups
StaticSemaphore_t _Mutex;
SemaphoreHandle_t i2cMutex = xSemaphoreCreateMutexStatic(&_Mutex);

void _lockMutex(void) { xSemaphoreTake(i2cMutex, portMAX_DELAY); }
void _unlockMutex(void) { xSemaphoreGive(i2cMutex); }

void i2c_setup(uint8_t i2cIndex, uint32_t baudrate, uint32_t timeout)
{
  if (i2cIndex >= WIRE_INTERFACES_COUNT)
  {
    assert(false);
    return;
  }
  auto wire = interfaces[i2cIndex];

  // begin before all, then set parameters
  wire->begin();
  // set parameters
  wire->setClock(baudrate);
  wire->setTimeout(timeout);
}

int i2c_check_existence(uint8_t i2cIndex, uint8_t deviceAddr)
{
  if (i2cIndex >= WIRE_INTERFACES_COUNT)
  {
    return 1;
  }
  _lockMutex();
  auto wire = interfaces[i2cIndex];

  wire->beginTransmission(deviceAddr);
  const auto res = wire->endTransmission();

  _unlockMutex();

  return res;
}

int i2c_writeData(uint8_t i2cIndex, uint8_t deviceAddr, uint8_t registerAdd, uint8_t size, uint8_t* buf, int stopBit)
{
  if (i2cIndex >= WIRE_INTERFACES_COUNT)
  {
    assert(false);
    return 1;
  }
  _lockMutex();
  auto wire = interfaces[i2cIndex];

  wire->beginTransmission(deviceAddr);
  wire->write(registerAdd);
  const uint8_t written = wire->write(buf, size);
  wire->endTransmission(stopBit != 0);

  _unlockMutex();

  return 0;
}

int i2c_readData(uint8_t i2cIndex, uint8_t deviceAddr, uint8_t registerAdd, uint8_t size, uint8_t* buf, int stopBit)
{
  if (i2cIndex >= WIRE_INTERFACES_COUNT)
  {
    assert(false);
    return 1;
  }
  _lockMutex();
  auto wire = interfaces[i2cIndex];

  wire->beginTransmission(deviceAddr);
  wire->write(registerAdd);
  wire->endTransmission(stopBit != 0);
  wire->requestFrom(deviceAddr, size);
  uint8_t count = size;
  while (wire->available() && count > 0)
  {
    *buf++ = wire->read();
    count--;
  }
  _unlockMutex();

  // return 0 for success
  return (count == 0) ? 0 : 1;
}

int i2c_xfer(
        uint8_t i2cIndex, uint8_t deviceAddr, int out_size, const uint8_t* out, int in_size, uint8_t* in, uint8_t flags)
{
  if (i2cIndex >= WIRE_INTERFACES_COUNT)
  {
    assert(false);
    return 1;
  }

  _lockMutex();
  auto wire = interfaces[i2cIndex];

  if (out_size)
  {
    wire->beginTransmission(deviceAddr);
    for (; out_size > 0; out_size--)
    {
      wire->write(*out);
      out++;
    }
    wire->endTransmission((flags & I2C_XFER_STOP) != 0);
  }

  if (in_size)
  {
    wire->requestFrom(deviceAddr, in_size, (flags & I2C_XFER_STOP));
    for (; in_size > 0; in_size--)
    {
      *in = wire->read();
      in++;
    }
  }
  _unlockMutex();

  return 0;
}

#endif
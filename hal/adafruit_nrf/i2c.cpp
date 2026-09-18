#include "src/system/hal/i2c.h"

#include "src/system/hal/time.h"
#include "src/system/hal/mutex.h"

#include <cassert>
#include <stdint.h>

// platform specific code
#include <Arduino.h>
#include "Wire.h"

namespace lampda {
namespace hal {
namespace i2c {

// set the two interfaces
TwoWire* PROGMEM interfaces[] = {&Wire};
bool PROGMEM isInit[] = {false};

// mutex to prevent i2c lockups
static hal::mutex::hal_mutex_t _I2CMutex;

void i2c_setup(uint8_t i2cIndex, uint32_t baudrate, uint32_t timeout)
{
  if (i2cIndex >= WIRE_INTERFACES_COUNT or isInit[i2cIndex])
  {
    assert(false);
    return;
  }

  if (hal::mutex::hal_mutex_init(&_I2CMutex) != HAL_MUTEX_OK)
  {
    return;
  }

  auto wire = interfaces[i2cIndex];

  // begin before all, then set parameters
  wire->begin();
  // set parameters
  wire->setClock(baudrate);
  wire->setTimeout(timeout);

  isInit[i2cIndex] = true;
}

void i2c_turn_off(uint8_t i2cIndex)
{
  if (i2cIndex >= WIRE_INTERFACES_COUNT)
  {
    assert(false);
    return;
  }
  // no need to turn off an unitiliazied class
  if (not isInit[i2cIndex])
  {
    return;
  }

  hal::mutex::hal_mutex_lock(&_I2CMutex);

  auto wire = interfaces[i2cIndex];
  isInit[i2cIndex] = false;
  wire->end();

  hal::mutex::hal_mutex_unlock(&_I2CMutex);
}

int i2c_check_existence(uint8_t i2cIndex, uint8_t deviceAddr)
{
  if (i2cIndex >= WIRE_INTERFACES_COUNT or not isInit[i2cIndex])
  {
    return 1;
  }

  hal::mutex::hal_mutex_lock(&_I2CMutex);

  auto wire = interfaces[i2cIndex];

  wire->beginTransmission(deviceAddr);
  const auto res = wire->endTransmission();

  hal::mutex::hal_mutex_unlock(&_I2CMutex);

  return res;
}

int lock_i2c()
{
  hal::mutex::hal_mutex_lock(&_I2CMutex);
  return 1;
}
int unlock_i2c()
{
  hal::mutex::hal_mutex_unlock(&_I2CMutex);
  return 1;
}

int i2c_writeData(
        uint8_t i2cIndex, uint8_t deviceAddr, uint8_t registerAdd, uint8_t size, const uint8_t* buf, int stopBit)
{
  if (i2cIndex >= WIRE_INTERFACES_COUNT or not isInit[i2cIndex])
  {
    assert(false);
    return 1;
  }

  hal::mutex::hal_mutex_lock(&_I2CMutex);

  auto wire = interfaces[i2cIndex];

  wire->beginTransmission(deviceAddr);
  wire->write(registerAdd);
  const uint8_t written = wire->write(buf, size);
  wire->endTransmission(stopBit != 0);

  hal::mutex::hal_mutex_unlock(&_I2CMutex);

  return 0;
}

int i2c_readData(uint8_t i2cIndex, uint8_t deviceAddr, uint8_t registerAdd, uint8_t size, uint8_t* buf, int stopBit)
{
  if (i2cIndex >= WIRE_INTERFACES_COUNT or not isInit[i2cIndex])
  {
    assert(false);
    return 1;
  }

  hal::mutex::hal_mutex_lock(&_I2CMutex);

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
  hal::mutex::hal_mutex_unlock(&_I2CMutex);

  // return 0 for success
  return (count == 0) ? 0 : 1;
}

int i2c_xfer_unlocked(
        uint8_t i2cIndex, uint8_t deviceAddr, int out_size, const uint8_t* out, int in_size, uint8_t* in, uint8_t flags)
{
  if (i2cIndex >= WIRE_INTERFACES_COUNT or not isInit[i2cIndex])
  {
    assert(false);
    return 1;
  }
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
  return 0;
}

} // namespace i2c
} // namespace hal
} // namespace lampda

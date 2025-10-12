#include "src/system/platform/i2c.h"

#define PLATFORM_I2C_CPP

#include <cstdint>
#include <memory>
#include <thread>
#include <atomic>

#include "src/system/platform/i2c.h"
#include "src/system/platform/time.h"

#include "simulator/mocks/electrical/i_ic.h"
#include "simulator/mocks/electrical/BQ25713_mock.h"

static constexpr size_t numberOfMocks = 1;
const std::unique_ptr<IntegratedCircuitMock_I> icMocks[numberOfMocks] = {
        // charger component
        std::make_unique<BQ25713Mock>(),
        // other
};
bool isI2cAvailable = false;

namespace mock_electrical {
// output at the power rail
float powerRailVoltage;
float powerRailCurrent;
// output at the led output
float outputVoltage;
float outputCurrent;
// output on vbus rail
float vbusVoltage;
float vbusCurrent;

float inputVbusVoltage;
float chargeOtgOutput;
} // namespace mock_electrical

std::atomic<bool> canRunComponentUpdateThread = false;
std::thread componentUpdateThread;

void i2c_setup(uint8_t i2cIndex, uint32_t baudrate, uint32_t timeout)
{
  if (i2cIndex != 0)
    return;

  canRunComponentUpdateThread = true;
  componentUpdateThread = std::thread([&]() {
    while (canRunComponentUpdateThread)
    {
      for (const auto& icMock: icMocks)
      {
        icMock->run_electrical_update();
      }
      delay_ms(1);
    }
  });
  isI2cAvailable = true;
}

void i2c_turn_off(uint8_t i2cIndex)
{
  if (i2cIndex != 0)
    return;

  canRunComponentUpdateThread = false;
  isI2cAvailable = false;
}

int i2c_check_existence(uint8_t i2cIndex, uint8_t deviceAddr)
{
  if (i2cIndex != 0 or !isI2cAvailable)
    return 1;

  for (const auto& icMock: icMocks)
  {
    if (icMock->get_i2c_address() == deviceAddr)
      return 0; // success
  }
  return 1;
}

int lock_i2c() { return 0; }
int unlock_i2c() { return 0; }

int i2c_writeData(uint8_t i2cIndex, uint8_t deviceAddr, uint8_t registerAdd, uint8_t size, uint8_t* buf, int stopBit)
{
  if (i2cIndex != 0 or !isI2cAvailable)
    return 1;

  for (auto& icMock: icMocks)
  {
    if (icMock->get_i2c_address() == deviceAddr)
      return icMock->i2c_write_data(registerAdd, size, buf);
  }
  return 1;
}

int i2c_readData(uint8_t i2cIndex, uint8_t deviceAddr, uint8_t registerAdd, uint8_t size, uint8_t* buf, int stopBit)
{
  if (i2cIndex != 0 or !isI2cAvailable)
    return 1;

  for (auto& icMock: icMocks)
  {
    if (icMock->get_i2c_address() == deviceAddr)
      return icMock->i2c_read_data(registerAdd, size, buf);
  }
  for (uint8_t i = 0; i < size; i++)
    buf[i] = 0;
  return 1;
}

int i2c_xfer_unlocked(
        uint8_t i2cIndex, uint8_t deviceAddr, int out_size, const uint8_t* out, int in_size, uint8_t* in, uint8_t flags)
{
  if (i2cIndex != 0 or !isI2cAvailable)
    return 1;

  for (auto& icMock: icMocks)
  {
    if (icMock->get_i2c_address() == deviceAddr)
      return icMock->i2c_xfer_data(out_size, out, in_size, in);
  }

  return 1;
}

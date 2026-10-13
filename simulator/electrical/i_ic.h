/*! \file i_ic.h
    \brief Electrical simulation interface with the electrical simulator
*/

#ifndef MOCK_I_IC_HPP
#define MOCK_I_IC_HPP

/**
 * This file defines the common interface for all IC mock.
 * They should have access to the electrical simulator, and an I2C interface
 */

#include <cstdint>

#include <memory>

namespace simulator {

class IntegratedCircuitMock_I
{
public:
  // run an update of the object properties
  virtual void run_electrical_update() = 0;

  // the i2c adress of the device
  virtual uint8_t get_i2c_address() const = 0;

  // write data to this device
  virtual int i2c_write_data(const uint8_t registerAddress, const uint8_t dataSize, const uint8_t* dataBuffer)
  {
    if (_registerMap[registerAddress] != nullptr)
    {
      // if (dataSize > 4) return 1;

      uint32_t data = 0;
      for (size_t i = 0; i < dataSize; i++)
      {
        data |= (dataBuffer[i] << (8 * i));
      }
      return _registerMap[registerAddress]->write(data);
    }

    // failure
    return 1;
  }

  // read data from a register
  virtual int i2c_read_data(const uint8_t registerAddress, const uint8_t dataSize, uint8_t* dataBuffer)
  {
    if (_registerMap[registerAddress] != nullptr)
    {
      // if (dataSize > 4) return 1;

      const uint32_t d = _registerMap[registerAddress]->read();
      for (size_t i = 0; i < dataSize; i++)
      {
        dataBuffer[i] = (d >> 8 * i) & 0xff;
      }
      return 0;
    }

    // failure
    return 1;
  }

  // simultaneous read write
  virtual int i2c_xfer_data(const int outSize, const uint8_t* out, const int inSize, uint8_t* in)
  {
    // fail
    return 1;
  }

  // register entry point
  struct Register
  {
    uint32_t _data = 0;

    virtual uint32_t read() { return _data; }
    virtual int write(uint32_t data)
    {
      _data = data;
      return 0;
    }
  };

protected:
  std::unique_ptr<Register> _registerMap[255];

private:
};

static constexpr size_t numberOfMocks = 2;
extern const std::array<std::unique_ptr<IntegratedCircuitMock_I>, numberOfMocks> icMocks;

} // namespace simulator

#endif

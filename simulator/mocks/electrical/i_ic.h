#ifndef MOCK_I_IC_HPP
#define MOCK_I_IC_HPP

/**
 * This file defines the common interface for all IC mock.
 * They should have access to the electrical simulator, and an I2C interface
 */

#include <cstdint>

class IntegratedCircuitMock_I
{
public:
  // run an update of the object properties
  virtual void run_electrical_update() = 0;

  // the i2c adress of the device
  virtual uint8_t get_i2c_address() const = 0;

  // write data to this device
  virtual int i2c_write_data(const uint8_t registerAddress, const uint8_t dataSize, const uint8_t* dataBuffer) = 0;
  // read data from a register
  virtual int i2c_read_data(const uint8_t registerAddress, const uint8_t dataSize, uint8_t* dataBuffer) = 0;
  // simultaneous read write
  virtual int i2c_xfer_data(const int outSize, const uint8_t* out, const int inSize, uint8_t* in) = 0;

protected:
private:
};

#endif

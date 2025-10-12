// do not use pragma once here, has this can be mocked
#ifndef PLATFORM_I2C
#define PLATFORM_I2C

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

  static const uint8_t pdNegociationI2cAddress = 0x22;
  static const uint8_t chargeI2cAddress = 0x6B;
  static const uint8_t imuI2cAddress = 0x6A;
  static const uint8_t batteryBalancerI2cAddress = 0x08;

/* Flags for i2c_xfer() */
#define I2C_XFER_START  (1 << 0)                         /* Start smbus session from idle state */
#define I2C_XFER_STOP   (1 << 1)                         /* Terminate smbus session with stop bit */
#define I2C_XFER_SINGLE (I2C_XFER_START | I2C_XFER_STOP) /* One transaction */

  /**
   * Define the general i2c interface (in c style for compatibility)
   * ALL FUNCTION RETURN 0 IN CASE OF SUCCESS !!
   * THEY RETURN 1 IN FAILURE
   * This is C style, not C++
   */

  /**
   * \brief Setup the i2c interface, needs to be called at program start
   * \param[in] i2cIndex The index of the i2c interface (from 0 to WIRE_INTERFACES_COUNT - 1)
   * \param[in] baudrate The baud rate of this interface (100000, 250000, 400000) in hertz
   * \param[in] timeout The tiemout in milliseconds after which a read or write fails
   */
  extern void i2c_setup(uint8_t i2cIndex, uint32_t baudrate, uint32_t timeout);

  /**
   * \brief Turn of the i2c interface, all i2c functions will return an error after that, until i2c_setup is called
   * again
   * \param[in] i2cIndex The index of the i2c interface (from 0 to WIRE_INTERFACES_COUNT - 1)
   */
  extern void i2c_turn_off(uint8_t i2cIndex);

  /**
   * \brief Return 0 if the address exists on the I2C line
   */
  extern int i2c_check_existence(uint8_t i2cIndex, uint8_t deviceAddr);

  int lock_i2c();
  int unlock_i2c();

  /**
   * \brief Write data to the two wire interface
   * \param[in] i2cIndex The index of the i2c interface (from 0 to WIRE_INTERFACES_COUNT - 1)
   * \param[in] deviceAddr the address of the target device
   * \param[in] registerAdd The address of the register to write
   * \param[in] size the size of the data to write (in bytes)
   * \param[in,out] buf the data to write, in an array of size \ref size
   * \param[in] stopBit if > 0, will add a stopBit after message
   */
  extern int i2c_writeData(
          uint8_t i2cIndex, uint8_t deviceAddr, uint8_t registerAdd, uint8_t size, uint8_t* buf, int stopBit);

  /**
   * \brief Read data from the two wire interface
   * \param[in] i2cIndex The index of the i2c interface (from 0 to WIRE_INTERFACES_COUNT - 1)
   * \param[in] deviceAddr the address of the target device
   * \param[in] registerAdd The address of the register to read
   * \param[in] size the size of the data to read (in bytes)
   * \param[in,out] buf the data to read, in an array of size \ref size
   * \param[in] stopBit if > 0, will add a stopBit after message
   */
  extern int i2c_readData(
          uint8_t i2cIndex, uint8_t deviceAddr, uint8_t registerAdd, uint8_t size, uint8_t* buf, int stopBit);

  /**
   * \brief Does a range read/write
   * \param[in] i2cIndex The index of the i2c interface (from 0 to WIRE_INTERFACES_COUNT - 1)
   * \param[in] deviceAddr the address of the target device
   * \param[in] out_size Size of the \ref out buffer to read
   * \param[out] out The buffer that will contain the read data
   * \param[in] in_size Size of the \ref in buffer to write
   * \param[in] in The buffer that contains the write data
   * \param[in] flags Flags to set the stop bit, start/stop info, etc
   */
  extern int i2c_xfer_unlocked(uint8_t i2cIndex,
                               uint8_t deviceAddr,
                               int out_size,
                               const uint8_t* out,
                               int in_size,
                               uint8_t* in,
                               uint8_t flags);

  /**
   * \brief Does a range read/write
   * \param[in] i2cIndex The index of the i2c interface (from 0 to WIRE_INTERFACES_COUNT - 1)
   * \param[in] deviceAddr the address of the target device
   * \param[in] out_size Size of the \ref out buffer to read
   * \param[out] out The buffer that will contain the read data
   * \param[in] in_size Size of the \ref in buffer to write
   * \param[in] in The buffer that contains the write data
   * \param[in] flags Flags to set the stop bit, start/stop info, etc
   */
  inline int i2c_xfer(uint8_t i2cIndex,
                      uint8_t deviceAddr,
                      int out_size,
                      const uint8_t* out,
                      int in_size,
                      uint8_t* in,
                      uint8_t flags)
  {
    lock_i2c();
    const int res = i2c_xfer_unlocked(i2cIndex, deviceAddr, out_size, out, in_size, in, flags);
    unlock_i2c();
    return res;
  }

  // define simple low weight handler

  /**
   * \brief Read 8bits from the two wire interface
   * \param[in] i2cIndex The index of the i2c interface (from 0 to WIRE_INTERFACES_COUNT - 1)
   * \param[in] deviceAddr the address of the target device
   * \param[in] registerAdd The address of the register to read
   * \param[in] val The value read from register
   * \param[in] stopBit if > 0, will add a stopBit after message
   */
  inline int i2c_read8(uint8_t i2cIndex, uint8_t deviceAddr, uint8_t registerAdd, uint8_t* val, int stopBit)
  {
    return i2c_readData(i2cIndex, deviceAddr, registerAdd, 1, val, stopBit);
  }

  /**
   * \brief Write 8bits from the two wire interface
   * \param[in] i2cIndex The index of the i2c interface (from 0 to WIRE_INTERFACES_COUNT - 1)
   * \param[in] deviceAddr the address of the target device
   * \param[in] registerAdd The address of the register to write
   * \param[in] val The value written to register
   * \param[in] stopBit if > 0, will add a stopBit after message
   */
  inline int i2c_write8(uint8_t i2cIndex, uint8_t deviceAddr, uint8_t registerAdd, uint8_t val, int stopBit)
  {
    return i2c_writeData(i2cIndex, deviceAddr, registerAdd, 1, &val, stopBit);
  }

  /**
   * \brief Read 16bits from the two wire interface
   * \param[in] i2cIndex The index of the i2c interface (from 0 to WIRE_INTERFACES_COUNT - 1)
   * \param[in] deviceAddr the address of the target device
   * \param[in] registerAdd The address of the register to read
   * \param[in] val The value read from register
   * \param[in] stopBit if > 0, will add a stopBit after message
   */
  inline int i2c_read16(uint8_t i2cIndex, uint8_t deviceAddr, uint8_t registerAdd, uint16_t* val, int stopBit)
  {
    return i2c_readData(i2cIndex, deviceAddr, registerAdd, 2, (uint8_t*)val, stopBit);
  }

  /**
   * \brief Write 16bits from the two wire interface
   * \param[in] i2cIndex The index of the i2c interface (from 0 to WIRE_INTERFACES_COUNT - 1)
   * \param[in] deviceAddr the address of the target device
   * \param[in] registerAdd The address of the register to write
   * \param[in] val The value written to register
   * \param[in] stopBit if > 0, will add a stopBit after message
   */
  inline int i2c_write16(uint8_t i2cIndex, uint8_t deviceAddr, uint8_t registerAdd, uint16_t val, int stopBit)
  {
    uint8_t data[2];
    data[0] = val & 0xFF;
    data[1] = (val >> 8) & 0xFF;
    return i2c_writeData(i2cIndex, deviceAddr, registerAdd, 2, data, stopBit);
  }

#ifdef __cplusplus
}
#endif

#endif

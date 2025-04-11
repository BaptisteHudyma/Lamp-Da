/*
 * tcpm_driver.c
 *
 * Created: 11/11/2017 18:42:26
 *  Author: jason
 */

#include "tcpm_driver.h"

extern const struct tcpc_config_t tcpc_config;

constexpr uint8_t i2cDeviceIndex = 0;

// TODO: replace return 0 by return true result

/* I2C wrapper functions - get I2C port / slave addr from config struct. */
int tcpc_write(int reg, int val)
{
  i2c_write8(tcpc_config.i2c_host_port, tcpc_config.i2c_slave_addr, reg & 0xFF, (uint8_t)val & 0xFF, 1);
  return 0;
}

int tcpc_write16(int reg, int val)
{
  i2c_write16(tcpc_config.i2c_host_port, tcpc_config.i2c_slave_addr, reg & 0xFF, (uint16_t)val, 1);
  return 0;
}

int tcpc_read(int reg, int* val)
{
  i2c_read8(tcpc_config.i2c_host_port, tcpc_config.i2c_slave_addr, reg & 0xFF, (uint8_t*)val, 0);
  return 0;
}

int tcpc_read16(int reg, int* val)
{
  i2c_read16(tcpc_config.i2c_host_port, tcpc_config.i2c_slave_addr, reg & 0xFF, (uint16_t*)val, 0);
  return 0;
}

int tcpc_xfer(const uint8_t* out, int out_size, uint8_t* in, int in_size, int flags)
{
  i2c_xfer(tcpc_config.i2c_host_port, tcpc_config.i2c_slave_addr, out_size, out, in_size, in, flags);
  return 0;
}
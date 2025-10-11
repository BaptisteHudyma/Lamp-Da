/*
 * tcpm_driver.c
 *
 * Created: 11/11/2017 18:42:26
 *  Author: jason
 */

#include "tcpm_driver.h"

extern const struct tcpc_config_t tcpc_config;

/* I2C wrapper functions - get I2C port / slave addr from config struct. */
int tcpc_write(int reg, int val)
{
  return i2c_write8(tcpc_config.i2c_host_port, tcpc_config.i2c_slave_addr, reg & 0xFF, (uint8_t)val & 0xFF, 1);
}

int tcpc_read(int reg, int* val)
{
  return i2c_read8(tcpc_config.i2c_host_port, tcpc_config.i2c_slave_addr, reg & 0xFF, (uint8_t*)val, 0);
}

int tcpc_xfer(const uint8_t* out, int out_size, uint8_t* in, int in_size, int flags)
{
  return i2c_xfer(tcpc_config.i2c_host_port, tcpc_config.i2c_slave_addr, out_size, out, in_size, in, flags);
}

int tcpc_xfer_unlocked(const uint8_t* out, int out_size, uint8_t* in, int in_size, int flags)
{
  return i2c_xfer_unlocked(tcpc_config.i2c_host_port, tcpc_config.i2c_slave_addr, out_size, out, in_size, in, flags);
}

void tcpc_lock(int lock)
{
  // TODO check result ?
  if (lock == 0)
    unlock_i2c();
  else
    lock_i2c();
}

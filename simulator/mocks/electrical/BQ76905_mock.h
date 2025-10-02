#ifndef BQ76905_MOCK_H
#define BQ76905_MOCK_H

#include "i_ic.h"

// hardwaree influencer simulator
#include "simulator/include/hardware_influencer.h"

#include "src/system/platform/print.h"
#include "src/system/platform/gpio.h"

#include <map>
#include <memory>

class BQ76905Mock : public IntegratedCircuitMock_I
{
public:
  BQ76905Mock()
  {
    // fill register map
    //_registerMap[bq76905::CHARGE_OPTION_0_ADDR] = std::make_unique<Register>();
  }

  void run_electrical_update() override
  {
    // TODO: update battery voltages
  }

  uint8_t get_i2c_address() const override { return bq76905::BQ76905::BQ76905addr; }

protected:
private:
  // Create instance of registers data structure
  inline static bq76905::BQ76905::Regt IcRegisters;
};

#endif

/*! \file BQ76905_mock.h
    \brief Define a mock of the BQ76905 electrical component.
*/

#ifndef BQ76905_MOCK_H
#define BQ76905_MOCK_H

#include "i_ic.h"

// use depend of real component (to have the registers)
#include "src/system/hal/i2c.h"
#include "src/depends/BQ76905/BQ76905.h"

// hardwaree influencer simulator
#include "simulator/include/hardware_influencer.h"

#include "src/system/hal/gpio.h"

#include "src/system/bsp/text_out.h"

#include <cstdint>
#include <map>
#include <memory>

namespace simulator {

class BQ76905Mock : public IntegratedCircuitMock_I
{
public:
  BQ76905Mock()
  {
    // fill register map
    _registerMap[bq76905::SAFETY_ALERT_A_ADDR] = std::make_unique<Register>();
    _registerMap[bq76905::SAFETY_ALERT_B_ADDR] = std::make_unique<Register>();

    _registerMap[bq76905::BATTERY_STATUS_ADDR] = std::make_unique<Register>();

    _registerMap[bq76905::ALARM_STATUS_ADDR] = std::make_unique<AlarmStatusRegister>();
    _registerMap[bq76905::ALARM_RAW_STATUS_ADDR] = std::make_unique<Register>();
    _registerMap[bq76905::ALARM_ENABLE_ADDR] = std::make_unique<Register>();

    _registerMap[bq76905::FET_CONTROL_ADDR] = std::make_unique<Register>();
    _registerMap[bq76905::REGOUT_CONTROL_ADDR] = std::make_unique<Register>();

    _registerMap[bq76905::DSG_FET_DRIVER_PWM_CONTROL_ADDR] = std::make_unique<Register>();
    _registerMap[bq76905::CHG_FET_DRIVER_PWM_CONTROL_ADDR] = std::make_unique<Register>();

    _registerMap[bq76905::CELL_1_VOLTAGE_ADDR] = std::make_unique<StandardCellVoltage_Register>();
    _registerMap[bq76905::CELL_2_VOLTAGE_ADDR] = std::make_unique<StandardCellVoltage_Register>();
    _registerMap[bq76905::CELL_3_VOLTAGE_ADDR] = std::make_unique<StandardCellVoltage_Register>();
    _registerMap[bq76905::CELL_4_VOLTAGE_ADDR] = std::make_unique<InvalidCellVoltage_Register>();
    _registerMap[bq76905::CELL_5_VOLTAGE_ADDR] = std::make_unique<InvalidCellVoltage_Register>();

    _registerMap[bq76905::REG_18_VOLTAGE_ADDR] = std::make_unique<Register>();
    _registerMap[bq76905::VSS_VOLTAGE_ADDR] = std::make_unique<Register>();
    _registerMap[bq76905::STACK_VOLTAGE_ADDR] = std::make_unique<StackVoltage_Register>();

    _registerMap[bq76905::INT_TEMPERATURE_ADDR] = std::make_unique<InternalTemp_Register>();
    _registerMap[bq76905::TS_MEASURMENT_ADDR] = std::make_unique<Register>();

    // subcommands

    // Main read place
    _registerMap[bq76905::SUBCOMMAND_ADDR] = std::make_unique<Subcommand_Register>(); // enter a subcommand
    _registerMap[bq76905::SUBCOMMAND_DATA_ADDR] =
            std::make_unique<SubcommandData_Register>(); // read/write the subcommand data
    _registerMap[bq76905::SUBCOMMAND_DATA_CHECKSUM_ADDR] = std::make_unique<Register>(); // check read/write validity
  }

  void run_electrical_update() override
  {
    // TODO: update battery voltages
  }

  uint8_t get_i2c_address() const override { return bq76905::BQ76905::BQ76905addr; }

protected:
  /// Bit list register
  struct AlarmStatusRegister : public Register
  {
    uint32_t read() override
    {
      uint8_t val0 = 0;
      val0 |= 1 << 0x07; // FULLSCAN
      val0 |= 0 << 0x06; // ADSCAN
      val0 |= 0 << 0x05; // WAKE
      val0 |= 0 << 0x04; // SLEEP
      val0 |= 0 << 0x03; // TIMER_ALARM
      val0 |= 0 << 0x02; // INITCOMP
      val0 |= 0 << 0x01; // CDTOGGLE
      val0 |= 0 << 0x0;  // POR

      uint8_t val1 = 0;
      val1 |= 0 << 0x06; // SSB
      val1 |= 0 << 0x05; // SAA
      val1 |= 0 << 0x04; // SAB
      val1 |= 0 << 0x03; // XCHG
      val1 |= 0 << 0x02; // XDSG
      val1 |= 0 << 0x01; // SHUTV
      val1 |= 1 << 0x0;  // CB

      uint32_t bitList = val0 | val1 << 8;
      return bitList;
    }
  };

  /// Temperature
  struct InternalTemp_Register : public Register
  {
    uint32_t read() override { return mock_registers::cpuTemperature; }
  };

  /// Stack voltage
  struct StackVoltage_Register : public Register
  {
    uint32_t read() override { return mock_battery::voltage * 1000.0; }
  };

  /// return the voltage on Cell 1
  struct StandardCellVoltage_Register : public Register
  {
    uint32_t read() override { return mock_battery::voltage * 1000.0 / 3; }
  };
  struct InvalidCellVoltage_Register : public Register
  {
    uint32_t read() override { return 0; }
  };

  /// Subcommand register
  struct Subcommand_Register : public Register
  {
    int write(uint32_t data) override
    {
      if (data > UINT16_MAX)
        return 1;

      BQ76905Mock::isInSubcommandState = true;
      BQ76905Mock::subcommandAddress = data;
      return 0;
    }
  };

  /// Subcommand data read
  struct SubcommandData_Register : public Register
  {
    uint32_t read() override
    {
      // check subcommand address, set data accordingly
      uint32_t answer = 0;
      if (not BQ76905Mock::isInSubcommandState)
        return 0;

      switch (BQ76905Mock::subcommandAddress)
      {
        case bq76905::SUBCOMMAND_DEVICE_NUMBER_ADDR:
          // big endian answer for device version
          answer = bq76905::DEVICE_NUMBER;
          break;
        case bq76905::SUBCOMMAND_FW_VERSION_ADDR:
          break;
        case bq76905::SUBCOMMAND_HW_VERSION_ADDR:
          break;
        case bq76905::SUBCOMMAND_PASSQ_ADDR:
          break;
        case bq76905::SUBCOMMAND_RESET_PASSQ_ADDR:
          break;
        case bq76905::SUBCOMMAND_EXIT_DEEPSLEEP_ADDR:
          break;
        case bq76905::SUBCOMMAND_DEEPSLEEP_ADDR:
          break;
        case bq76905::SUBCOMMAND_RESET_ADDR:
          break;
        case bq76905::SUBCOMMAND_FET_ENABLE_ADDR:
          break;
        case bq76905::SUBCOMMAND_SEAL_ADDR:
          break;
        case bq76905::SUBCOMMAND_SECURITY_KEYS_ADDR:
          break;
        case bq76905::SUBCOMMAND_CB_ACTIVE_CELLS_ADDR:
          break;
        case bq76905::SUBCOMMAND_SET_CFGUPDATE_ADDR:
          break;
        case bq76905::SUBCOMMAND_EXIT_CFGUPDATE_ADDR:
          break;
        case bq76905::SUBCOMMAND_PROG_TIMER_ADDR:
          break;
        case bq76905::SUBCOMMAND_SLEEP_ENABLE_ADDR:
          break;
        case bq76905::SUBCOMMAND_SLEEP_DISABLE_ADDR:
          break;
        case bq76905::SUBCOMMAND_PROT_RECOVERY_ADDR:
          break;
        case bq76905::SUBCOMMAND_CONFIGURATION_DA_ADDR:
          break;
        case bq76905::SUBCOMMAND_CONFIGURATION_VCELL_MODE_ADDR:
          break;
        case bq76905::SUBCOMMAND_CONFIGURATION_TS_OFFSET_ADDR:
          break;
      }
      BQ76905Mock::isInSubcommandState = false;
      return answer;
    }
  };

private:
  // Create instance of registers data structure
  inline static bq76905::BQ76905::Regt IcRegisters;

  inline static bool isInSubcommandState = false;
  inline static uint16_t subcommandAddress = 0;
};

} // namespace simulator

#endif

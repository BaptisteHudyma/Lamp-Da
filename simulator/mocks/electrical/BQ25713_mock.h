#ifndef BQ25713_MOCK_H
#define BQ25713_MOCK_H

#include "i_ic.h"

// use depend of real component (to have the registers)
#include "depends/BQ25713/BQ25713.h"

// hardwaree influencer simulator
#include "simulator/include/hardware_influencer.h"

#include "src/system/utils/utils.h"

#include "src/system/platform/print.h"
#include "src/system/platform/gpio.h"

#include <map>

// independant of enabl/disable
float targetOTGVoltage = 0.0;
namespace __private {
DigitalPin enableOTG(DigitalPin::GPIO::Output_EnableOnTheGo);
}

class BQ25713Mock : public IntegratedCircuitMock_I
{
public:
  BQ25713Mock()
  {
    // fill register map

    // ids
    _registerMap[bq25713::MANUFACTURER_ID_ADDR] = std::make_unique<ManufacturerId>();
    _registerMap[bq25713::DEVICE_ID_ADDR] = std::make_unique<DeviceId>();

    // simple read bit
    _registerMap[bq25713::CHARGE_OPTION_0_ADDR] = std::make_unique<Register>();
    _registerMap[bq25713::CHARGE_OPTION_1_ADDR] = std::make_unique<Register>();
    _registerMap[bq25713::CHARGE_OPTION_2_ADDR] = std::make_unique<Register>();
    _registerMap[bq25713::CHARGE_OPTION_3_ADDR] = std::make_unique<Register>();

    _registerMap[bq25713::ADC_OPTION_ADDR] = std::make_unique<AdcOption_Register>();
    _registerMap[bq25713::PROCHOT_STATUS_ADDR] = std::make_unique<Register>();

    _registerMap[bq25713::PROCHOT_OPTION_0_ADDR] = std::make_unique<Register>();
    _registerMap[bq25713::PROCHOT_OPTION_1_ADDR] = std::make_unique<Register>();

    // status
    _registerMap[bq25713::CHARGE_STATUS_ADDR] = std::make_unique<ChargerStatus_Register>();
    //
    _registerMap[bq25713::VBAT_ADC_ADDR] = std::make_unique<VBAT_ADC_Register>();
    _registerMap[bq25713::CHARGE_CURRENT_ADDR] = std::make_unique<Register>();
    _registerMap[bq25713::MAX_CHARGE_VOLTAGE_ADDR] = std::make_unique<Register>();
    _registerMap[bq25713::MINIMUM_SYSTEM_VOLTAGE_ADDR] = std::make_unique<Register>();
    _registerMap[bq25713::OTG_VOLTAGE_ADDR] = std::make_unique<OTG_Register>();
    _registerMap[bq25713::OTG_CURRENT_ADDR] = std::make_unique<Register>();
    _registerMap[bq25713::INPUT_VOLTAGE_ADDR] = std::make_unique<Register>();
    _registerMap[bq25713::IIN_HOST_ADDR] = std::make_unique<Register>();
    _registerMap[bq25713::IIN_DPM_ADDR] = std::make_unique<Register>();

    _registerMap[bq25713::ADC_VBUS_PSYS_ADC_ADDR] = std::make_unique<AdcVbusPsys_Register>();
    _registerMap[bq25713::ADC_IBAT_ADDR] = std::make_unique<Register>();
    _registerMap[bq25713::CMPIN_ADC_ADDR] = std::make_unique<Register>();
  }

  void run_electrical_update() override
  {
    // TODO

    // check OTG enable
    const uint16_t chargeOption3 = _registerMap[bq25713::CHARGE_OPTION_3_ADDR]->read();
    if (chargeOption3)
    {
      const uint8_t val0 = chargeOption3 & 0xff;
      const uint8_t val1 = (chargeOption3 >> 8) & 0xff;
      // EN_OTG
      if ((val1 & (1 << 0x04)) != 0 && __private::enableOTG.is_high())
      {
        mock_electrical::chargeOtgOutput = targetOTGVoltage;
      }
      else
      {
        mock_electrical::chargeOtgOutput = 0;
      }
    }
  }

  uint8_t get_i2c_address() const override { return bq25713::BQ25713::BQ25713addr; }

protected:
private:
  // Create instance of registers data structure
  inline static bq25713::BQ25713::Regt IcRegisters;

  // interface with base registers
  static uint16_t encode_to_base_read_register(const uint16_t val, const bq25713::BQ25713::IBaseReadRegister* reg)
  {
    // constraint
    uint16_t valC = (lmpd_constrain<uint16_t>(val, reg->minVal(), reg->maxVal()) - reg->minVal()) / reg->resolution();
    // rectified
    uint16_t valR = (valC & reg->mask()) << reg->offset();
    return valR;
  }
  static uint16_t decode_base_register_value(const uint16_t val, const bq25713::BQ25713::IBaseReadRegister* reg)
  {
    // unpack
    uint16_t res = val;
    res >>= reg->offset();
    res &= reg->mask();
    // convert to real units
    return res * reg->resolution() + reg->minVal();
  }

  static uint16_t encode_to_double_register(const uint16_t val0,
                                            const uint16_t val1,
                                            const bq25713::BQ25713::IDoubleRegister* reg)
  {
    // rectified vals
    uint8_t val0R = (max<uint16_t>(val0, reg->minVal0()) - reg->minVal0()) / reg->resolutionVal0();
    uint8_t val1R = (max<uint16_t>(val1, reg->minVal1()) - reg->minVal1()) / reg->resolutionVal1();

    uint16_t result = (val1R & reg->maskVal1()) << 8 | (val0R & reg->maskVal0());
    return result;
  }

  struct ChargerStatus_Register : public Register
  {
    uint16_t read() override
    {
      uint8_t val0 = 0;

      uint8_t val1 = 0;
      // val1 |= 1 << 0x07;  // AC stat
      val1 |= 1 << 0x06; // ICO done
      val1 |= 1 << 0x00; // in OTG

      return val1 << 8 | val0;
    }
  };

  struct AdcOption_Register : public Register
  {
    uint16_t read() override
    {
      uint8_t val0 = 0;

      uint8_t val1 = 0;
      val1 |= 0 << 0x06; // ADC_START

      return val1 << 8 | val0;
    }
  };

  struct OTG_Register : public Register
  {
    int write(uint16_t data) override
    {
      const uint16_t decoded = decode_base_register_value(data, &IcRegisters.oTGVoltage);

      // adjust for low range
      targetOTGVoltage = (decoded + (IcRegisters.chargeOption3.OTG_RANGE_LOW() ? 0 : 1280) + 154) / 1000.0;

      _data = data;
      return 0;
    }
  };

  struct AdcVbusPsys_Register : public Register
  {
    uint16_t read() override
    {
      return encode_to_double_register(
              mock_battery::voltage * 1000.0, mock_electrical::powerRailVoltage * 1000.0, &IcRegisters.aDCVBUSPSYS);
    }
  };

  // Battery and Vsys register
  struct VBAT_ADC_Register : public Register
  {
    uint16_t read() override
    {
      return encode_to_double_register(
              mock_battery::voltage * 1000.0, mock_battery::voltage * 1000.0, &IcRegisters.aDCVSYSVBAT);
    }
  };

  struct ManufacturerId : public Register
  {
    uint16_t read() override { return bq25713::MANUFACTURER_ID; }
  };

  struct DeviceId : public Register
  {
    uint16_t read() override { return bq25713::DEVICE_ID; }
  };
};

#endif

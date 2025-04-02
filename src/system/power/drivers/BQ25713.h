/**************************************************************************/
/*!
  @file     BQ25713.h
  @author   Baptiste Hudyma

Library for basic interfacing with BQ25713 battery management IC from TI

Based on Lorro's implementation for BQ25703A driver

*/
/**************************************************************************/

#ifndef BQ25713_H
#define BQ25713_H

#include "src/system/platform/i2c.h"

#include "src/system/utils/utils.h"
#include <cstdint>

namespace bq25713 {

using byte = uint8_t;

// Platform specific
constexpr uint8_t i2cObjectIndex = 0;
// \Platform specific

constexpr bool usesStopBit = true;

constexpr uint16_t DEVICE_ID = 0x88;
constexpr uint16_t MANUFACTURER_ID = 0x40;

constexpr uint16_t CHARGE_CURRENT_ADDR = 0x02;
constexpr uint16_t MAX_CHARGE_VOLTAGE_ADDR = 0x04;
constexpr uint16_t MINIMUM_SYSTEM_VOLTAGE_ADDR = 0x0C;

constexpr uint16_t OTG_VOLTAGE_ADDR = 0x06;
constexpr uint16_t OTG_CURRENT_ADDR = 0x08;

constexpr uint16_t INPUT_VOLTAGE_ADDR = 0x0A;
constexpr uint16_t IIN_HOST_ADDR = 0x0E;
constexpr uint16_t IIN_DPM_ADDR = 0x24;

constexpr uint16_t CHARGE_OPTION_0_ADDR = 0x00;
constexpr uint16_t CHARGE_OPTION_1_ADDR = 0x30;
constexpr uint16_t CHARGE_OPTION_2_ADDR = 0x32;
constexpr uint16_t CHARGE_OPTION_3_ADDR = 0x34;

constexpr uint16_t PROCHOT_OPTION_0_ADDR = 0x36;
constexpr uint16_t PROCHOT_OPTION_1_ADDR = 0x38;

constexpr uint16_t ADC_OPTION_ADDR = 0x3A;
constexpr uint16_t CHARGE_STATUS_ADDR = 0x20;
constexpr uint16_t PROCHOT_STATUS_ADDR = 0x22;
constexpr uint16_t ADC_VBUS_PSYS_ADC_ADDR = 0x26;
constexpr uint16_t ADC_IBAT_ADDR = 0x28;
constexpr uint16_t CMPIN_ADC_ADDR = 0x2A;
constexpr uint16_t VBAT_ADC_ADDR = 0x2C;

constexpr uint16_t MANUFACTURER_ID_ADDR = 0x2E;
constexpr uint16_t DEVICE_ID_ADDR = 0x2F;

constexpr uint16_t OTG_ENABLE_CELL_LOW_VOLTAGE =
        3550; // minimum single cell voltage at which the OTG will refuse to start

class BQ25713
{
public:
  BQ25713() {};
  // Initialise the variable here, but it will be written from the main program
  static const byte BQ25713addr = chargeI2cAddress; // I2C address

  template<typename T> bool readRegEx(T& dataParam)
  {
    // This is a function for reading data words.
    // The number of bytes that make up a word is 2.
    constexpr uint8_t arrLen = 2;
    // Create an array to hold the returned data
    byte valBytes[arrLen];

    if (readDataReg(dataParam.addr, valBytes, arrLen))
    {
      // Cycle through array of data
      dataParam.val0 = (byte)valBytes[0];
      dataParam.val1 = (byte)valBytes[1];
      return true;
    }
    return false;
  }
  // used by external functions to write registers
  template<typename T> static bool writeRegEx(T dataParam)
  {
    byte valBytes[2];
    valBytes[0] = dataParam.val0;
    valBytes[1] = dataParam.val1;
    return i2c_writeData(i2cObjectIndex, BQ25713addr, dataParam.addr, 2, valBytes, usesStopBit ? 1 : 0) == 0;
  }
// macro to generate bit mask to access bits
#define GETMASK(index, size) (((1 << (size)) - 1) << (index))
// macro to read bits from variable, using mask
#define READFROM(data, index, size) (((data) & GETMASK((index), (size))) >> (index))
// macro to write bits into variable, using mask
#define WRITETO(data, index, size, value) ((data) = ((data) & (~GETMASK((index), (size)))) | ((value) << (index)))
// macro to wrap functions for easy access
// if name is called with empty brackets, read bits and return value
// if name is prefixed with set_, write value in brackets into bits defined in
// FIELD
#define FIELD(data, name, index, size)                                 \
  inline decltype(data) name() { return READFROM(data, index, size); } \
  inline void set_##name(decltype(data) value) { WRITETO(data, index, size, value); }

// macro to wrap functions for easy access, read only
// if name is called with empty brackets, read bits and return value
#define FIELD_RO(data, name, index, size) \
  inline decltype(data) name() { return READFROM(data, index, size); }

  // used to set and reset the error flag
  inline static bool isFlagRaised = false;

  // Base class for register operations
  struct IBaseReadRegister
  {
    virtual ~IBaseReadRegister() {};

    virtual uint16_t address() const = 0;

    virtual uint16_t minVal() const = 0;
    virtual uint16_t maxVal() const = 0;
    virtual uint16_t resolution() const = 0;

    virtual uint8_t bitLenght() const = 0;
    virtual uint8_t offset() const = 0;

    uint16_t mask() const
    {
      // convert a bit count to a bit mask
      uint16_t mask = 1;
      mask <<= (bitLenght() + 1);
      return mask - 1;
    }

    uint16_t get()
    {
      byte valBytes[2];
      if (readDataReg(address(), valBytes, 2))
      {
        // Cycle through array of data
        byte val0 = (byte)valBytes[0];
        byte val1 = (byte)valBytes[1];

        // fuse them
        uint16_t res = val1 << 8 | val0;
        // unpack
        res >>= offset();
        res &= mask();
        // convert to real units
        return res * resolution() + minVal();
      }
      isFlagRaised = true;
      // error
      return 0;
    }
  };

  struct IBaseRegister : public IBaseReadRegister
  {
    uint16_t set(const uint16_t value)
    {
      // convert to authorized values, break it down to the correct resolution (integer division)
      const uint16_t constraint = (lmpd_constrain(value, minVal(), maxVal()) - minVal()) / resolution();

      // convert to binary word
      uint16_t binaryWord = constraint;
      binaryWord &= mask();    // mask off unused bits (with &= for 16bits)
      binaryWord <<= offset(); // offset the register (with <<= for 16 bits)

      if (!writeData16(address(), binaryWord))
      {
        isFlagRaised = true;
      }

      // return the value that was written to the register
      return (constraint * resolution()) + minVal();
    }
  };

  // Base class for distinct stored value operations
  struct IDoubleRegister
  {
    virtual uint16_t address() const = 0;

    virtual uint16_t minVal0() const = 0;
    virtual uint16_t maskVal0() const = 0;
    virtual uint16_t resolutionVal0() const = 0;

    virtual uint16_t minVal1() const = 0;
    virtual uint16_t maskVal1() const = 0;
    virtual uint16_t resolutionVal1() const = 0;

    uint16_t getVal0()
    {
      byte valBytes[2];
      if (readDataReg(address(), valBytes, 2))
      {
        // Cycle through array of data
        byte val0 = (byte)valBytes[0];
        // fuse them
        uint16_t res = val0 & maskVal0();
        // convert to real units
        return res * resolutionVal0() + minVal0();
      }
      isFlagRaised = true;
      // error
      return 0;
    }

    uint16_t getVal1()
    {
      byte valBytes[2];
      if (readDataReg(address(), valBytes, 2))
      {
        // Cycle through array of data
        byte val1 = (byte)valBytes[1];
        // fuse them
        uint16_t res = val1 & maskVal1();
        // convert to real units
        return res * resolutionVal1() + minVal1();
      }
      isFlagRaised = true;
      // error
      return 0;
    }
  };

  struct Regt
  {
    struct ChargeOption0t
    {
      uint8_t addr = CHARGE_OPTION_0_ADDR;
      byte val0 = 0x0E;
      byte val1 = 0x82;
      // To disable the hiccup mode during the system short protection
      FIELD(val0, SYS_SHORT_DISABLE, 0x06, 0x01)
      // Learn mode. Discharges with power connected. Default disabled
      FIELD(val0, EN_LEARN, 0x05, 0x01)
      // Current shunt amplifier 20x or 40x. Default is 20x
      FIELD(val0, IADPT_GAIN, 0x04, 0x01)
      // Bat current shunt amplifier 8x or 16x. Default is 16x
      FIELD(val0, IBAT_GAIN, 0x03, 0x01)
      // LDO mode - use of pre charge. Default is precharge enabled
      FIELD(val0, EN_LDO, 0x02, 0x01)
      // Enable IDPM current control. Default is high(enabled)
      FIELD(val0, EN_IDPM, 0x01, 0x01)
      // Inhibit charging. Default is low(enabled)
      FIELD(val0, CHRG_INHIBIT, 0x00, 0x01)
      // Enable low power mode. Default is enabled
      FIELD(val1, EN_LWPWR, 0x07, 0x01)
      // Watchdog timer. Reset it by setting ChargeCurrent  or
      // MaxChargeVoltage commands 00b: Disable Watchdog Timer 01b: Enabled, 5
      // sec 10b: Enabled, 88 sec 11b: Enable Watchdog Timer, 175 sec <default
      // at POR>
      FIELD(val1, WDTMR_ADJ, 0x05, 0x02)
      // Disable IDPM. Default is low (IDPM enabled)
      FIELD(val1, IDPM_AUTO_DISABLE, 0x04, 0x01)
      // Turn Chrgok on if OTG is enabled. Default is low
      FIELD(val1, OTG_ON_CHRGOK, 0x03, 0x01)
      // Out of audio switch frequency. Default is low(disabled)
      FIELD(val1, EN_OOA, 0x02, 0x01)
      // PWM switching frequency, 800kHz or 1.2MHz. Default is high (800kHz)
      FIELD(val1, PWM_FREQ, 0x01, 0x01)
      // PTM mode input voltage and current ripple reduction
      FIELD(val1, LOW_PTM_RIPPLE, 0x00, 0x01)
    } chargeOption0;
    struct ChargeOption1t
    {
      uint8_t addr = CHARGE_OPTION_1_ADDR;
      byte val0 = 0x11;
      byte val1 = 0x02;
      // Internal comparator reference 2.3V or 1.2V. Default is 2.3V
      FIELD(val0, CMP_REF, 0x07, 0x01)
      // Internal comparator polarity
      FIELD(val0, CMP_POL, 0x06, 0x01)
      // Internal comparator deglitch time; off, 1us, 2ms, 5s. Default is 1us
      FIELD(val0, CMP_DEG, 0x04, 0x02)
      // Force power path to switch off. Default is disabled
      FIELD(val0, FORCE_LATCHOFF, 0x03, 0x01)
      // Discharge SRN pin for shipping. Default is disabled
      FIELD(val0, EN_SHIP_DCHG, 0x01, 0x01)
      // Automatically charge for 30mins at 128mA if voltage is below min.
      // Default enabled.
      FIELD(val0, AUTO_WAKEUP_EN, 0x00, 0x01)
      // Enable IBAT output buffer. Default is disabled
      FIELD(val1, EN_IBAT, 0x07, 0x01)
      // PROCHOT during battery only; disabled, IDCHG, VSYS. Default is
      // disabled
      FIELD(val1, EN_PROCHOT_LPWR, 0x05, 0x02)
      // Enable PSYS buffer. Default is disabled
      FIELD(val1, EN_PSYS, 0x04, 0x01)
      // Charge sense resistor; 10mR or 20mR. Default is 10mR
      FIELD(val1, RSNS_RAC, 0x03, 0x01)
      // Input sense resistor; 10mR or 20mR. Default is 10mR
      FIELD(val1, RSNS_RSR, 0x02, 0x01)
      // PSYS gain; 0.25uA/W or 1uA/W. Default is 1uA/W
      FIELD(val1, PSYS_RATIO, 0x01, 0x01)
      // elect the ILIM_HIZ pin function
      // 0b: charger enters HIZ mode when pull low the ILIM_HIZ pin. <default at POR>
      // 1b: charger enters PTM when pull low the ILIM_HIZ pin
      FIELD(val1, PTM_PINSEL, 0x00, 0x01)
    } chargeOption1;
    struct ChargeOption2t
    {
      uint8_t addr = CHARGE_OPTION_2_ADDR;
      byte val0 = 0xB7;
      byte val1 = 0x02;
      // Allow ILIM_HIZ pin to set current limit. Default is enabled
      FIELD(val0, EN_EXTILIM, 0x07, 0x01)
      // Function of IBAT pin; discharge or charge. Default is discharge
      FIELD(val0, EN_ICHG_IDCHG, 0x06, 0x01)
      // Over Current Protection for Q2 by sensing VDS; 210mV or 150mV.
      // Default is 150mV
      FIELD(val0, Q2_OCP, 0x05, 0x01)
      // Over Current Protection for input between ACP and ACN; 210mV or
      // 150mV. default is 150mV
      FIELD(val0, ACX_OCP, 0x04, 0x01)
      // Input adapter OCP enable. Default is disabled
      FIELD(val0, EN_ACOC, 0x03, 0x01)
      // Input adapter OCP disabled current limit; 125% of ICRIT or 210% of
      // ICRIT. Default is 210%
      FIELD(val0, ACOC_VTH, 0x02, 0x01)
      // Bat OCP; disabled or related to PROCHOT IDCHG. Default is IDPM
      FIELD(val0, EN_BATOC, 0x01, 0x01)
      // OCP related to PROCHOT IDCHG; 125% or 200%. Default is 200%
      FIELD(val0, BATOC_VTH, 0x00, 0x01)
      // Input overload time; 1ms, 2mS, 10mS, 20mS. Default is 1mS
      FIELD(val1, PKPWR_TOVLD_DEG, 0x06, 0x02)
      // Enable peak power mode from over current. Default is disabled
      FIELD(val1, EN_PKPWR_IDPM, 0x05, 0x01)
      // Enable peak power mode from under voltage. Default is disabled
      FIELD(val1, EN_PKPWR_VSYS, 0x04, 0x01)
      // Indicator that device is in overloading cycle. Default disabled
      FIELD(val1, PKPWR_OVLD_STAT, 0x03, 0x01)
      // Indicator that device is in relaxation cycle. Default disabled
      FIELD(val1, PKPWR_RELAX_STAT, 0x02, 0x01)
      // Peak power mode overload and relax cycle times; 5mS, 10mS, 20mS,
      // 40mS. Default is 20mS
      FIELD(val1, PKPWR_TMAX, 0x00, 0x02)
    } chargeOption2;
    struct ChargeOption3t
    {
      uint8_t addr = CHARGE_OPTION_3_ADDR;
      byte val0 = 0x00;
      byte val1 = 0x00;
      // Enable the conservative VAP mode.
      // 0b: Disabled <default at POR>
      // 1b: Enabled
      FIELD(val0, EN_CON_VAP, 0x06, 0x01)
      // The selection of the external OTG/VAP pin control.
      // 0b: the external OTG/VAP pin controls the EN/DIS VAP mode
      // 1b: the external OTG/VAP pin controls the EN/DIS OTG mode <default at POR>
      FIELD(val0, OTG_VAP_MODE, 0x05, 0x01)
      // 4 levels inductor average current clamp.
      // 00b: 6A
      // 01b: 10A
      // 10b: 15A <default at POR>
      // 11b: Disabled
      FIELD(val0, IL_AVG, 0x03, 0x02)
      // Selection of the different OTG output voltage range.
      // 0b: VOTG high range 4.28 V - 20.8 V <default at POR>
      // 1b: VOTG low range 3 V - 19.52 V
      FIELD(val0, OTG_RANGE_LOW, 0x02, 0x01)
      // Control BAT FET during Hi-Z state. Default is disabled
      FIELD(val0, BATFETOFF_HIZ, 0x01, 0x01)
      // PSYS function during OTG mode. PSYS = battery discharge - IOTG or
      // PSYS = battery discharge. Default 0
      FIELD(val0, PSYS_OTG_IDCHG, 0x00, 0x01)
      // Enable Hi-Z(low power) mode. Default is disabled
      FIELD(val1, EN_HIZ, 0x07, 0x01)
      // Reset registers. Set this bit to 1 to reset all other registers
      FIELD(val1, RESET_REG, 0x06, 0x01)
      // Reset VINDPM register. Default is idle (0)
      FIELD(val1, RESET_VINDPM, 0x05, 0x01)
      // Enable OTG mode to output power to VBUS. EN_OTG pin needs to be high.
      // Default is disabled.
      FIELD(val1, EN_OTG, 0x04, 0x01)
      // Enable Input Current Optimiser. Default is disabled
      FIELD(val1, EN_ICO_MODE, 0x03, 0x01)
    } chargeOption3;
    struct ProchotOption0t
    {
      uint8_t addr = PROCHOT_OPTION_0_ADDR;
      byte val0 = 0x50;
      byte val1 = 0x92;
      // VSYS Threshold to trigger discharging VBUS in VAP mode.
      // Measure on VSYS with fixed 5-μs deglitch time. Trigger when SYS pin voltage is
      // below the thresholds.
      // 2S - 4S battery
      // 0000b - 1111b: 5.9 V - 7.4V with 0.1 V step size.
      // 1S battery
      // 0000b - 0111b: 3.1 V - 3.8 V with 0.1 V step size.
      // 1000b - 1111b: 3.1 V - 3.8 V with 0.1 V step size.
      FIELD(val0, VSYS_TH1, 0x04, 0x03)
      // VSYS Threshold to assert /PROCHOT_VSYS.
      // Measure on VSYS with fixed 5-μs deglitch time. Trigger when SYS pin voltage is
      // below the thresholds.
      // 2S - 4S battery
      // 00b: 5.9V; 01b: 6.2V <default at POR>;
      // 10b: 6.5V; 11b: 6.8V.
      // 1S battery
      // 00b: 3.1V; 01b: 3.3V <default at POR>;
      // 10b: 3.5V; 11b: 3.7V
      FIELD(val0, VSYS_TH2, 0x02, 0x02)
      // INOM deglitch time; 1ms or 50ms. Default is 1ms
      FIELD(val0, INOM_DEG, 0x01, 0x01)
      // Enable the lower threshold of the PROCHOT_VDPM comparator
      // 0b: the threshold of the PROCHOT_VDPM comparator follows the same VinDPM
      // REG0x0A/0B() setting.
      // 1b: the threshold of the PROCHOT_VDPM comparator is lower and determined by
      // REG0x37[0] setting. <default at POR>
      FIELD(val0, LOWER_PROCHOT_VDPM, 0x00, 0x01)
      // ILIM2 threshold as percentage of IDPM; 110%-230%(5% step),
      // 250%-450%(50% step). Default is 150%
      FIELD(val1, ILIM2_VTH, 0x03, 0x05)
      // ICRIT deglitch time. ICRIT is 110% of ILIM2; 15us, 100us, 400us,
      // 800us. Default is 100us.
      FIELD(val1, ICRIT_DEG, 0x01, 0x02)
      // Lower threshold of the PROCHOT_VDPM comparator
      // When REG0x36[0]=1, the threshold of the PROCHOT_VDPM comparator is
      // determined by this bit setting.
      // 0b: 80% of VinDPM threshold <default at POR>.
      // 1b: 90% of VinDPM threshold
      FIELD(val1, PROCHOT_VDPM_80_90, 0x00, 0x00)
    } prochotOption0;
    struct ProchotOption1t
    {
      uint8_t addr = PROCHOT_OPTION_1_ADDR;
      byte val0 = 0x20;
      byte val1 = 0x41;
      // When all the REG0x38[7:0] bits are 0, PROCHOT function is disabled.
      // Bit7 PP_VDPM detects VBUS voltage
      FIELD(val0, PROCHOT_PROFILE_VDPM, 0x07, 0x01)
      // PROCHOT profile comparator. Default is disabled.
      FIELD(val0, PROCHOT_PROFILE_COMP, 0x06, 0x01)
      // Prochot is triggered if ICRIT threshold is reached. Default enabled.
      FIELD(val0, PROCHOT_PROFILE_ICRIT, 0x05, 0x01)
      // Prochot is triggered if INOM threshold is reached. Default disabled.
      FIELD(val0, PROCHOT_PROFILE_INOM, 0x04, 0x01)
      // Enable battery Current Discharge current reading. Default is
      // disabled.
      FIELD(val0, PROCHOT_PROFILE_IDCHG, 0x03, 0x01)
      // Prochot is triggered if VSYS threshold is reached. Default disabled.
      FIELD(val0, PROCHOT_PROFILE_VSYS, 0x02, 0x01)
      // PROCHOT will be triggered if the battery is removed. Default is
      // disabled.
      FIELD(val0, PROCHOT_PROFILE_BATPRES, 0x01, 0x01)
      // PROCHOT will be triggered if the adapter is removed. Default is
      // disabled.
      FIELD(val0, PROCHOT_PROFILE_ACOK, 0x00, 0x01)
      // IDCHG threshold. PROCHOT is triggered when IDCHG is above; 0-32356mA
      // in 512mA steps. Default is 16384mA
      FIELD(val1, IDCHG_VTH, 0x02, 0x06)
      // IDCHG deglitch time; 1.6ms, 100us, 6ms, 12ms. Default is 100us.
      FIELD(val1, IDCHG_DEG, 0x00, 0x02)
    } prochotOption1;
    struct ADCOptiont
    {
      uint8_t addr = ADC_OPTION_ADDR;
      byte val0 = 0x00;
      byte val1 = 0x20;
      // Enable comparator voltage reading. Default is disabled.
      FIELD(val0, EN_ADC_CMPIN, 0x07, 0x01)
      // Enable VBUS voltage reading. Default is disabled.
      FIELD(val0, EN_ADC_VBUS, 0x06, 0x01)
      // Enable PSYS voltage reading for calculating system power. Default is
      // disabled.
      FIELD(val0, EN_ADC_PSYS, 0x05, 0x01)
      // Enable Current In current reading. Default is disabled.
      FIELD(val0, EN_ADC_IIN, 0x04, 0x01)
      // Enable battery Current Discharge current reading. Default is
      // disabled.
      FIELD(val0, EN_ADC_IDCHG, 0x03, 0x01)
      // Enable battery Current Charge current reading. Default is disabled.
      FIELD(val0, EN_ADC_ICHG, 0x02, 0x01)
      // Enable Voltage of System voltage reading. Default is disabled.
      FIELD(val0, EN_ADC_VSYS, 0x01, 0x01)
      // Enable Voltage of Battery voltage reading. Default is disabled.
      FIELD(val0, EN_ADC_VBAT, 0x00, 0x01)
      // ADC mode; one shot reading or continuous. Default is one shot
      FIELD(val1, ADC_CONV, 0x07, 0x01)
      // Start a one shot reading of the ADC. Resets to 0 after reading
      FIELD(val1, ADC_START, 0x06, 0x01)
      // ADC scale; 2.04V or 3.06V. Default is 3.06V
      FIELD(val1, ADC_FULLSCALE, 0x05, 0x01)
    } aDCOption;
    struct ChargerStatust
    {
      uint8_t addr = CHARGE_STATUS_ADDR;
      byte val0, val1;
      // Latched fault flag of adapter over voltage. Default is no fault.
      FIELD_RO(val0, Fault_ACOV, 0x07, 0x01)
      // Latched fault flag of battery over current. Default is no fault.
      FIELD_RO(val0, Fault_BATOC, 0x06, 0x01)
      // Latched fault flag of adapter over current. Default is no fault.
      FIELD_RO(val0, Fault_ACOC, 0x05, 0x01)
      // Latched fault flag of system over voltage protection. Default is no
      // fault.
      FIELD(val0, SYSOVP_STAT, 0x04, 0x01)
      // When SYS is lower than 2.4V, then 7 times restart tries are failed.
      FIELD(val0, Fault_SYS_SHORT, 0x03, 0x01)
      // Resets faults latch. Default is disabled
      FIELD_RO(val0, Fault_Latchoff, 0x02, 0x01)
      // Latched fault flag of OTG over voltage protection. Default is no
      // fault.
      FIELD_RO(val0, Fault_OTG_OVP, 0x01, 0x01)
      // Latched fault flag of OTG over current protection. Default is no
      // fault.
      FIELD_RO(val0, Fault_OTG_UVP, 0x00, 0x01)
      // Input source present. Default is not connected.
      FIELD_RO(val1, AC_STAT, 0x07, 0x01)
      // After ICO routine is done, bit goes to zero.
      FIELD_RO(val1, ICO_DONE, 0x06, 0x01)
      // Charger is not operated in VAP mode
      FIELD_RO(val1, IN_VAP, 0x05, 0x01)
      // Charger is in VINDPM or OTG mode. Default is not
      FIELD_RO(val1, IN_VINDPM, 0x04, 0x01)
      // Device is in current in DPM mode. Default is not
      FIELD_RO(val1, IN_IINDPM, 0x03, 0x01)
      // Device is in fast charge mode. Default is not
      FIELD_RO(val1, IN_FCHRG, 0x02, 0x01)
      // Device is in pre charge mode. Default is not
      FIELD_RO(val1, IN_PCHRG, 0x01, 0x01)
      // Device is in OTG mode. Default is not
      FIELD_RO(val1, IN_OTG, 0x00, 0x01)
    } chargerStatus;
    struct ProchotStatust
    {
      uint8_t addr = PROCHOT_STATUS_ADDR;
      byte val0, val1;
      // PROCHOT VDPM status. Default is not triggered.
      FIELD_RO(val0, STAT_VDPM, 0x07, 0x01)
      // PROCHOT comparator trigger status. Default is not triggered.
      FIELD_RO(val0, STAT_COMP, 0x06, 0x01)
      // PROCHOT current critical trigger status. Default is not triggered.
      FIELD_RO(val0, STAT_ICRIT, 0x05, 0x01)
      // PROCHOT input current exceeds 110% threshold trigger. Default is not
      // triggered.
      FIELD_RO(val0, STAT_INOM, 0x04, 0x01)
      // PROCHOT discharge current trigger status. Default is not triggered.
      FIELD_RO(val0, STAT_IDCHG, 0x03, 0x01)
      // PROCHOT system voltage trigger status. Default is not triggered.
      FIELD_RO(val0, STAT_VSYS, 0x02, 0x01)
      // PROCHOT battery removal trigger status. Default is not triggered.
      FIELD_RO(val0, STAT_Battery_Removal, 0x01, 0x01)
      // PROCHOT adapter removal trigger status. Default is not triggered.
      FIELD_RO(val0, STAT_Adapter_Removal, 0x00, 0x01)

      // PROCHOT Pulse Extension Enable. When pulse extension is
      // enabled, keep the PROCHOT pin voltage LOW until host writes
      // REG0x23[3] = 0.
      // 0b: Disable pulse extension <default at POR>
      // 1b: Enable pulse extension
      FIELD(val1, EN_PROCHOT_EXIT, 0x06, 0x01)
      // PROCHOT Pulse Width.
      // Minimum PROCHOT pulse width when REG0x23[6] = 0
      // 00b: 100 us
      // 01b: 1 ms
      // 10b: 10 ms <default at POR>
      // 11b: 5 ms
      FIELD(val1, PROCHOT_WIDTH, 0x04, 0x02)
      // PROCHOT Pulse Clear.
      // Clear PROCHOT pulse when 0x23[6] = 1.
      // 0b: Clear PROCHOT pulse and drive PROCHOT pin HIGH
      // 1b: Idle <default at POR>
      FIELD(val1, PROCHOT_CLEAR, 0x03, 0x01)
      // This status bit reports a failure to load VBUS 7 consecutive times
      // in VAP mode, which indicates the battery voltage might be not
      // high enough to enter VAP mode, or the VAP loading current
      // settings are too high.
      // 0b: Not is VAP failure <default at POR>
      // 1b: In VAP failure, the charger exits VAP mode, and latches off
      // until the host writes this bit to 0.
      FIELD(val1, STAT_VAP_FAIL, 0x01, 0x01)
      // When the charger is operated in VAP mode, it can exit VAP by
      // either being disabled through host, or there is any charger faults.
      // 0b: PROCHOT_EXIT_VAP is not active <default at POR>
      // 1b: PROCHOT_EXIT_VAP is active, PROCHOT pin is low until
      // host writes this status bit to 0.
      FIELD(val1, STAT_EXIT_VAP, 0x00, 0x01)
    } prochotStatus;
    struct ChargeCurrentt : public IBaseRegister
    {
      uint16_t address() const override { return CHARGE_CURRENT_ADDR; }

      virtual uint16_t minVal() const override { return 0; }
      virtual uint16_t maxVal() const override { return 8128; }
      virtual uint16_t resolution() const override { return 64; }

      virtual uint8_t bitLenght() const override { return 7; }
      virtual uint8_t offset() const override { return 6; }
    } chargeCurrent;
    struct MaxChargeVoltaget : public IBaseRegister
    {
      uint16_t address() const override { return MAX_CHARGE_VOLTAGE_ADDR; }

      // the min in the doc is set to 1024, but this is wrong in practice
      virtual uint16_t minVal() const override { return 0; }
      virtual uint16_t maxVal() const override { return 19200; }
      virtual uint16_t resolution() const override { return 8; }

      virtual uint8_t bitLenght() const override { return 12; }
      virtual uint8_t offset() const override { return 3; }
    } maxChargeVoltage;
    struct MinSystemVoltaget : public IBaseRegister
    {
      uint16_t address() const override { return MINIMUM_SYSTEM_VOLTAGE_ADDR; }

      // the min in the doc is set to 1024, but this is wrong in practice
      virtual uint16_t minVal() const override { return 0; }
      virtual uint16_t maxVal() const override { return 16128; }
      virtual uint16_t resolution() const override { return 256; }

      virtual uint8_t bitLenght() const override { return 6; }
      virtual uint8_t offset() const override { return 8; }
    } minSystemVoltage;

    struct IIN_HOSTt : public IBaseRegister
    {
      uint16_t address() const override { return IIN_HOST_ADDR; }

      virtual uint16_t minVal() const override { return 50; }
      virtual uint16_t maxVal() const override { return 6400; }
      virtual uint16_t resolution() const override { return 50; }

      virtual uint8_t bitLenght() const override { return 7; }
      virtual uint8_t offset() const override { return 8; }
    } iIN_HOST;
    // IIN_DPM register reflects the actual input current limit programmed in
    // the register, either from host or from ICO.
    struct IIN_DPMt : public IBaseReadRegister
    {
      uint16_t address() const override { return IIN_DPM_ADDR; }

      virtual uint16_t minVal() const override { return 50; }
      virtual uint16_t maxVal() const override { return 6400; }
      virtual uint16_t resolution() const override { return 50; }

      virtual uint8_t bitLenght() const override { return 7; }
      virtual uint8_t offset() const override { return 8; }
    } iIN_DPM;
    // If the input voltage drops more than the InputVoltage register allows,
    // the device enters DPM and reduces the charge current
    struct InputVoltaget : public IBaseRegister
    {
      uint16_t address() const override { return INPUT_VOLTAGE_ADDR; }

      virtual uint16_t minVal() const override { return 3200; }
      virtual uint16_t maxVal() const override { return 19584; }
      virtual uint16_t resolution() const override { return 64; }

      virtual uint8_t bitLenght() const override { return 8; }
      virtual uint8_t offset() const override { return 6; }
    } inputVoltage;
    struct OTGVoltaget : public IBaseRegister
    {
      uint16_t address() const override { return OTG_VOLTAGE_ADDR; }

      uint16_t get(bool isLowVoltageMode)
      {
        // set min value depending on low otg voltage mode
        minValue = isLowVoltageMode ? 0 : 1280;

        return IBaseRegister::get();
      }

      void set(uint16_t val, bool isLowVoltageMode)
      {
        // set min value depending on low otg voltage mode
        minValue = isLowVoltageMode ? 0 : 1280;

        // TODO: why do the OTG voltage is always 184mV above what is requested
        IBaseRegister::set(val - 184);
      }

      // min val should be 3000mV, but in practise thats not it
      virtual uint16_t minVal() const override { return minValue; }
      virtual uint16_t maxVal() const override { return 20800; }
      virtual uint16_t resolution() const override { return 8; }

      virtual uint8_t bitLenght() const override { return 12; }
      virtual uint8_t offset() const override { return 2; }

      uint16_t minValue = 0;
    } oTGVoltage;
    // The OTGCurrent register is a limit after which the device will raise the
    // OTG_OVP flag
    struct OTGCurrentt : public IBaseRegister
    {
      uint16_t address() const override { return OTG_CURRENT_ADDR; }

      virtual uint16_t minVal() const override { return 0; }
      virtual uint16_t maxVal() const override { return 6400; }
      virtual uint16_t resolution() const override { return 50; }

      virtual uint8_t bitLenght() const override { return 7; }
      virtual uint8_t offset() const override { return 8; }
    } oTGCurrent;
    // Allows to read the VBUS & PSYS voltage from the ADC
    struct ADCVBUSPSYSt : public IDoubleRegister
    { // read only
      uint16_t address() const override { return ADC_VBUS_PSYS_ADC_ADDR; };

      uint16_t minVal0() const override { return 0; }; // TODO: check this
      uint16_t maskVal0() const override { return 0b11111111; }
      uint16_t resolutionVal0() const override { return 12; };

      uint16_t minVal1() const override { return 3200; };
      uint16_t maskVal1() const override { return 0b11111111; }
      uint16_t resolutionVal1() const override { return 64; };

      // input voltage on VBUS
      uint16_t get_VBUS() { return getVal1(); }
      // system power (exprimed as mV, which is strange...)
      uint16_t get_PSYS() { return getVal0(); }
    } aDCVBUSPSYS;
    // Allows to read the battery charge and discharge current from DAC
    struct ADCIBATt : public IDoubleRegister
    { // read only
      uint16_t address() const override { return ADC_IBAT_ADDR; };

      uint16_t minVal0() const override { return 0; };
      uint16_t maskVal0() const override { return 0b01111111; }
      uint16_t resolutionVal0() const override { return 256; };

      uint16_t minVal1() const override { return 0; };
      uint16_t maskVal1() const override { return 0b01111111; }
      uint16_t resolutionVal1() const override { return 64; };

      // ICHG battery charging current value
      uint16_t get_ICHG() { return getVal1(); }
      // IDCHG battery discharging current value
      uint16_t get_IDCHG() { return getVal0(); }
    } aDCIBAT;
    struct ADCIINCMPINt : public IDoubleRegister
    { // read only
      uint16_t address() const override { return CMPIN_ADC_ADDR; };

      uint16_t minVal0() const override { return 0; };
      uint16_t maskVal0() const override { return 0b11111111; }
      uint16_t resolutionVal0() const override { return 12; };

      uint16_t minVal1() const override { return 0; };
      uint16_t maskVal1() const override { return 0b11111111; }
      uint16_t resolutionVal1() const override { return 50; };

      uint16_t IIN, CMPIN;
      byte val0, val1;
      uint8_t addr = CMPIN_ADC_ADDR;
      // IIN input current reading
      uint16_t get_IIN() { return getVal1(); }
      // CMPIN voltage on comparator pin
      uint16_t get_CMPIN() { return getVal0(); }
    } aDCIINCMPIN;
    struct ADCVSYSVBATt : public IDoubleRegister
    { // read only
      uint16_t address() const override { return VBAT_ADC_ADDR; };

      uint16_t minVal0() const override { return 2880; };
      uint16_t maskVal0() const override { return 0b11111111; }
      uint16_t resolutionVal0() const override { return 64; };

      uint16_t minVal1() const override { return 2880; };
      uint16_t maskVal1() const override { return 0b11111111; }
      uint16_t resolutionVal1() const override { return 64; };

      // VSYS system voltage
      uint16_t get_VSYS() { return getVal1(); }
      // VBAT voltage of battery
      uint16_t get_VBAT() { return getVal0(); }
    } aDCVSYSVBAT;

    struct ManufacturerIDt
    { // read only
      // Manufacturer ID
      uint8_t addr = MANUFACTURER_ID_ADDR;
      byte get_manufacturerID()
      {
        byte valBytes[1];
        // Function to handle the I2C comms.
        readDataReg(addr, valBytes, 1);
        return (byte)valBytes[0];
      }
    } manufacturerID;
    struct DeviceID
    { // read only
      // Device ID
      uint8_t addr = DEVICE_ID_ADDR;
      byte get_deviceID()
      {
        byte valBytes[1];
        // Function to handle the I2C comms.
        readDataReg(addr, valBytes, 1);
        return (byte)valBytes[0];
      }
    } deviceID;
  };

private:
  static bool readDataReg(const byte regAddress, byte* dataVal, const uint8_t arrLen)
  {
    return i2c_readData(i2cObjectIndex, BQ25713addr, regAddress, arrLen, dataVal, usesStopBit ? 1 : 0) == 0;
  }

  static bool writeData16(const byte regAddress, uint16_t data)
  {
    return i2c_write16(i2cObjectIndex, BQ25713addr, regAddress, data, usesStopBit ? 1 : 0) == 0;
  }
};

} // namespace bq25713

#endif // BQ25713_H
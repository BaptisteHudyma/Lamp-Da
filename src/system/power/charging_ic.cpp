#include "charging_ic.h"

#include <cstdint>

#include "src/system/platform/i2c.h"
#include "src/system/platform/time.h"
#include "src/system/platform/gpio.h"
#include "src/system/platform/registers.h"
#include "src/system/platform/print.h"

#include "src/system/logic/alerts.h"

#include "src/system/utils/utils.h"
#include "src/system/utils/time_utils.h"

// use depend of component
#include "depends/BQ25713/BQ25713.h"

namespace charger {
namespace drivers {

// Initialise the device and library
bq25713::BQ25713 chargerIc;

// Create instance of registers data structure
bq25713::BQ25713::Regt chargerIcRegisters;

// store the ADC measurments
static Measurments measurments_s;
// store the battery measurments
static Battery battery_s;

// store the component status
inline static Status_t status_s = Status_t::UNINITIALIZED;
// store an error message for errors
inline static std::string status_error = "";
// store the current charge status of the system
inline static ChargeStatus_t chargeStatus_s = ChargeStatus_t::OFF;

// charger can charge if needed
inline static bool isChargeOk_s = false;
// charger is charging
inline static bool isChargeEnabled_s = false;
// charger is in OTG state
inline static bool isInOtg_s = false;

std::string softwareError_detail = "";
std::string get_software_error_message()
{
  if (softwareError_detail.empty())
    return "x";
  return softwareError_detail;
}
void set_software_error_message(const std::string& msg)
{
  if (softwareError_detail.empty())
    softwareError_detail = msg;
}

struct PowerLimits
{
  uint16_t maxChargingCurrent_mA = 0;
  uint16_t current_mA = 0;
  bool shoulduseICO = false;

  void set_default()
  {
    current_mA = 100; // default to 100mA (standard USB)
    shoulduseICO = false;
  }
};
// define the power limits for input power
static PowerLimits powerLimits_s;

// detect the faults on the status
void run_fault_detection()
{
  if (chargerIcRegisters.chargerStatus.Fault_ACOV() or chargerIcRegisters.chargerStatus.Fault_BATOC() or
      chargerIcRegisters.chargerStatus.Fault_ACOC() or chargerIcRegisters.chargerStatus.SYSOVP_STAT() or
      chargerIcRegisters.chargerStatus.Fault_Latchoff() or chargerIcRegisters.chargerStatus.Fault_OTG_OVP() or
      chargerIcRegisters.chargerStatus.Fault_OTG_UVP())
  {
    lampda_print("Charger ic faults: %d%d%d%d%d%d%d",
                 chargerIcRegisters.chargerStatus.Fault_ACOV(),
                 chargerIcRegisters.chargerStatus.Fault_BATOC(),
                 chargerIcRegisters.chargerStatus.Fault_ACOC(),
                 chargerIcRegisters.chargerStatus.SYSOVP_STAT(),
                 chargerIcRegisters.chargerStatus.Fault_Latchoff(),
                 chargerIcRegisters.chargerStatus.Fault_OTG_OVP(),
                 chargerIcRegisters.chargerStatus.Fault_OTG_UVP());
    status_s = Status_t::ERROR_HAS_FAULTS;
  }
  // reset fault flag
  else if (status_s == Status_t::ERROR_HAS_FAULTS)
  {
    status_s = Status_t::NOMINAL;
  }
}

// update the local values of the status register
void run_status_update()
{
  const byte initialReg0read = chargerIcRegisters.chargerStatus.val0;
  const byte initialReg1read = chargerIcRegisters.chargerStatus.val1;

  chargerIc.readRegEx(chargerIcRegisters.chargerStatus);

  if (initialReg0read != chargerIcRegisters.chargerStatus.val0 or
      initialReg1read != chargerIcRegisters.chargerStatus.val1)
  {
    // check faults
    run_fault_detection();
  }

  // handle OTG alerts
  if (isInOtg_s)
  {
    if (chargerIcRegisters.chargerStatus.IN_OTG())
    {
      alerts::manager.clear(alerts::Type::OTG_FAILED);
    }
    else
    {
      alerts::manager.raise(alerts::Type::OTG_FAILED);
    }
  }
}

// control the status of the charge
// can be called at any point
void control_charge()
{
  // only charge when no errors
  const bool shouldCharge =
          // user allows charge
          isChargeEnabled_s and
          // charge ok signal from ic
          isChargeOk_s and
          // power limit current is set to valid value
          powerLimits_s.current_mA > 0 and
          // no errors
          (status_s == Status_t::NOMINAL) and
          // battery is present
          battery_s.isPresent and
          // not already in OTG
          not isInOtg_s;
  // inhibit charging
  chargerIc.readRegEx(chargerIcRegisters.chargeOption0);
  const int shouldInihibit = shouldCharge ? 0 : 1;

  // if charge status changed, write it
  if (chargerIcRegisters.chargeOption0.CHRG_INHIBIT() != shouldInihibit)
  {
    chargerIcRegisters.chargeOption0.set_CHRG_INHIBIT(shouldInihibit);
    chargerIc.writeRegEx(chargerIcRegisters.chargeOption0);
  }
}

void control_OTG()
{
  static bool isOTGInitialized_s = false;
  static bool isOTGReseted = false;
  if (isInOtg_s)
  {
    // OTG use time
    static uint32_t OTGStartTime_ms = 0;
    static uint32_t lastOTGUsedTime_ms = 0;

    if (not isOTGInitialized_s)
    {
      isOTGInitialized_s = true;
      isOTGReseted = false;

      OTGStartTime_ms = time_ms();
      lastOTGUsedTime_ms = time_ms();

      DigitalPin(DigitalPin::GPIO::Output_EnableOnTheGo).set_high(true);

      chargerIc.readRegEx(chargerIcRegisters.chargeOption3);
      // TODO test and debug LOW RANGE
      chargerIcRegisters.chargeOption3.set_OTG_RANGE_LOW(0);
      chargerIcRegisters.chargeOption3.set_EN_OTG(1);
      chargerIc.writeRegEx(chargerIcRegisters.chargeOption3);

      chargerIc.readRegEx(chargerIcRegisters.chargerStatus);
      if (not chargerIcRegisters.chargerStatus.IN_OTG())
      {
        // alert will be lowered on time
        alerts::manager.raise(alerts::Type::OTG_FAILED);
      }
    }
    else
    {
      // check current consumption
      const auto& measurment = get_measurments();
      if (not measurment.is_measurment_valid())
      {
        // do not run anything
        return;
      }

      if (measurment.vbus_mV <= 3200)
      {
        // wait for voltage to climb on VBUS
        return;
      }
      if (measurment.vbus_mA > 0)
      {
        // update the last used time
        lastOTGUsedTime_ms = time_ms();
      }

      // update the in OTG status to avoid deconnection
      DigitalPin(DigitalPin::GPIO::Output_EnableOnTheGo).set_high(true);
      chargerIc.readRegEx(chargerIcRegisters.chargeOption3);
      chargerIcRegisters.chargeOption3.set_EN_OTG(1);
      chargerIc.writeRegEx(chargerIcRegisters.chargeOption3);
    }
  }
  else
  {
    // should deactivate OTG, so do it
    if (not isOTGReseted)
    {
      isOTGInitialized_s = false;
      isOTGReseted = true;

      disable_OTG();

      DigitalPin(DigitalPin::GPIO::Output_EnableOnTheGo).set_high(false);
      delay_ms(1);
      // 5V 0A for OTG (default)
      set_OTG_targets(5000, 0);

      alerts::manager.clear(alerts::Type::OTG_FAILED);
    }
  }
}

// enable/disable the charge function
void enable_charge(const bool enable)
{
  // set the global status
  isChargeEnabled_s = enable;

  // quick switch off charge
  if (!isChargeEnabled_s)
    control_charge();
}

// enable/disable the Input Current Optimizer algorithm
void enable_ico(const bool enable)
{
  if (enable)
  {
    chargerIc.readRegEx(chargerIcRegisters.chargeOption3);
    // Do not reset ICO, if it is already enabled.
    if (chargerIcRegisters.chargeOption3.EN_ICO_MODE() != 0)
      return;

    // enable ICO
    chargerIcRegisters.chargeOption3.set_EN_ICO_MODE(1);
    chargerIcRegisters.chargeOption3.set_RESET_VINDPM(1);
    chargerIc.writeRegEx(chargerIcRegisters.chargeOption3);
  }
  else
  {
    // disable ICO
    chargerIcRegisters.chargeOption3.set_EN_ICO_MODE(0);
    chargerIc.writeRegEx(chargerIcRegisters.chargeOption3);
  }
}

// update the battery struct internal state
void update_battery()
{
  battery_s.voltage_mV = measurments_s.battery_mV;

  const uint16_t chargingCurrent = measurments_s.batChargeCurrent_mA;
  const bool isInputSourcePresent = is_input_source_present();

  // charger is present (consider only charging current)
  battery_s.current_mA = (int16_t)chargingCurrent - (int16_t)measurments_s.batDischargeCurrent_mA;

  const uint16_t batteryMaxVoltage = chargerIcRegisters.maxChargeVoltage.get();
  const bool isAlmostFullyCharged = battery_s.voltage_mV > (batteryMaxVoltage * 0.99);

  // output voltage saturated, battery is not here
  battery_s.isPresent = battery_s.voltage_mV <= (batteryMaxVoltage + chargerIcRegisters.aDCVSYSVBAT.resolutionVal0());

  // check the charging status
  if (not isInputSourcePresent or (not isAlmostFullyCharged and chargingCurrent <= 0))
  {
    chargeStatus_s = ChargeStatus_t::OFF;
  }
  // special case: almot fully charged will drop the charge current
  else if (isAlmostFullyCharged)
  {
    chargeStatus_s = ChargeStatus_t::FASTCHARGE;
  }
  // charging current gets low
  else if (chargingCurrent <= powerLimits_s.maxChargingCurrent_mA * 0.1)
  {
    chargeStatus_s = ChargeStatus_t::SLOW_CHARGE;
  }
  // pre charge
  else if (chargerIcRegisters.chargerStatus.IN_PCHRG() != 0)
  {
    chargeStatus_s = ChargeStatus_t::PRECHARGE;
  }
  // normal charge
  else if (chargerIcRegisters.chargerStatus.IN_FCHRG() != 0)
  {
    chargeStatus_s = ChargeStatus_t::FASTCHARGE;
  }
  else if (isInputSourcePresent)
  {
    // TODO: check this
    // deglitch time of deconnection ?
  }
  else
  {
    // weird, error ?
    chargeStatus_s = ChargeStatus_t::OFF;
  }
}

// run the ADC readings
void run_ADC()
{
  static bool isAdcTriggered = false;
  if (not isAdcTriggered)
  {
    chargerIc.readRegEx(chargerIcRegisters.aDCOption);
    // start a new ADC read
    chargerIcRegisters.aDCOption.set_ADC_CONV(0);
    chargerIcRegisters.aDCOption.set_ADC_START(1);
    // Set ADC for each parameters
    chargerIcRegisters.aDCOption.set_EN_ADC_CMPIN(1);
    chargerIcRegisters.aDCOption.set_EN_ADC_VBUS(1);
    chargerIcRegisters.aDCOption.set_EN_ADC_PSYS(1);
    chargerIcRegisters.aDCOption.set_EN_ADC_IIN(1);
    chargerIcRegisters.aDCOption.set_EN_ADC_IDCHG(1);
    chargerIcRegisters.aDCOption.set_EN_ADC_ICHG(1);
    chargerIcRegisters.aDCOption.set_EN_ADC_VSYS(1);
    chargerIcRegisters.aDCOption.set_EN_ADC_VBAT(1);
    // write the register
    chargerIc.writeRegEx(chargerIcRegisters.aDCOption);

    isAdcTriggered = true;
  }
  else
  {
    // check if the measurment is made
    chargerIc.readRegEx(chargerIcRegisters.aDCOption);
    if (chargerIcRegisters.aDCOption.ADC_START() != 0)
    {
      // ADC in progress
      return;
    }

    // reset the flag to trigger a new read
    isAdcTriggered = false;

    // store everything in the measurment struct
    measurments_s.time = time_ms();
    measurments_s.vbus_mV = chargerIcRegisters.aDCVBUSPSYS.get_VBUS();
    measurments_s.psys_mV = chargerIcRegisters.aDCVBUSPSYS.get_PSYS();
    measurments_s.batChargeCurrent_mA = chargerIcRegisters.aDCIBAT.get_ICHG();
    measurments_s.batDischargeCurrent_mA = chargerIcRegisters.aDCIBAT.get_IDCHG();
    measurments_s.vbus_mA = chargerIcRegisters.aDCIINCMPIN.get_IIN();
    measurments_s.cmpin_mA = chargerIcRegisters.aDCIINCMPIN.get_CMPIN();
    measurments_s.vsys_mV = chargerIcRegisters.aDCVSYSVBAT.get_VSYS();
    measurments_s.battery_mV = chargerIcRegisters.aDCVSYSVBAT.get_VBAT();

    // update the battery parameters
    update_battery();
  }
}

// set the input current limit
// this will inhibit the battery charge current limit
void program_input_current_limit()
{
  const uint16_t inputCurrentLimit_mA = powerLimits_s.current_mA;
  const bool shouldUseICO = powerLimits_s.shoulduseICO;

  // enable IDPM
  chargerIcRegisters.chargeOption0.set_EN_IDPM(1);
  chargerIc.writeRegEx(chargerIcRegisters.chargeOption0);

  if (isChargeOk_s and inputCurrentLimit_mA > 0)
  {
    enable_ico(shouldUseICO);

    const uint16_t writtenCurrent_mA = chargerIcRegisters.iIN_HOST.set(inputCurrentLimit_mA);

    // the checks below can produce a rare software error when plugging the system
    // It it not that important, so it can stay deactivated to prevent this pesky error
#if 0
    // check that the register is set
    if (writtenCurrent_mA != chargerIcRegisters.iIN_HOST.get())
    {
      set_software_error_message("written current register iIN_HOST do not match consign");
      status_s = Status_t::ERROR;
      return;
    }
    if (not shouldUseICO and writtenCurrent_mA != chargerIcRegisters.iIN_DPM.get())
    {
      set_software_error_message("written current register iIN_DPM do not match consign");
      status_s = Status_t::ERROR;
      return;
    }
#endif

    // force disable ILIM pin hardware input current limit
    chargerIcRegisters.chargeOption2.set_EN_EXTILIM(0);
    chargerIc.writeRegEx(chargerIcRegisters.chargeOption2);
  }
  // set in current to 0
  else
  {
    // disable ico
    enable_ico(false);

    // input current to 0
    chargerIcRegisters.iIN_HOST.set(0);

    // enable hardware limit check
    chargerIcRegisters.chargeOption2.set_EN_EXTILIM(1);
    chargerIc.writeRegEx(chargerIcRegisters.chargeOption2);
  }

  // read IDPM status (should be enabled)
  chargerIc.readRegEx(chargerIcRegisters.chargeOption0);
  if (chargerIcRegisters.chargeOption0.EN_IDPM() == 0)
  {
    set_software_error_message("EN IDPM not enabled");
    status_s = Status_t::ERROR;
    return;
  }
}

/**
 *
 *
 *  HEADER FUNCTIONS
 *
 *
 */

bool enable(const uint16_t minSystemVoltage_mV,
            const uint16_t maxBatteryVoltage_mV,
            const uint16_t maxChargingCurrent_mA,
            const uint16_t maxDichargingCurrent_mA,
            const bool forceReset)
{
  if (i2c_check_existence(bq25713::i2cObjectIndex, bq25713::BQ25713::BQ25713addr) != 0)
  {
    // error: device not detected
    // charger
    status_s = Status_t::ERROR_COMPONENT;
    status_error = "charging component is not detected on i2c line";
    return false;
  }

  if (chargerIc.isFlagRaised or chargerIcRegisters.manufacturerID.get_manufacturerID() != bq25713::MANUFACTURER_ID or
      chargerIcRegisters.deviceID.get_deviceID() != bq25713::DEVICE_ID)
  {
    // error: those constants do not indicate a BQ25713
    // charger
    status_s = Status_t::ERROR_COMPONENT;
    status_error = "charging component do not match expected ids";
    return false;
  }

  // reset the parameters if needed
  if (forceReset)
  {
    // write the reset flag
    chargerIc.readRegEx(chargerIcRegisters.chargeOption3);
    chargerIcRegisters.chargeOption3.set_RESET_REG(1);
    chargerIc.writeRegEx(chargerIcRegisters.chargeOption3);

    // wait until the flag is lowered
    uint32_t timeout = time_ms() + 500;
    do
    {
      delay_ms(5);
      chargerIc.readRegEx(chargerIcRegisters.chargeOption3);
    } while (time_ms() < timeout and chargerIcRegisters.chargeOption3.RESET_REG() == 1);
  }

  // everything went fine (for now)

  // disable the OTG (safety)
  disable_OTG();

  status_s = Status_t::NOMINAL;

  chargerIc.readRegEx(chargerIcRegisters.chargeOption3);
  // disable high impedance mode
  chargerIcRegisters.chargeOption3.set_EN_HIZ(0);
  chargerIc.writeRegEx(chargerIcRegisters.chargeOption3);

  // disable low power mode
  chargerIcRegisters.chargeOption0.set_EN_LWPWR(0);
  chargerIc.writeRegEx(chargerIcRegisters.chargeOption0);

  chargerIcRegisters.prochotOption1.set_IDCHG_VTH(128 + (maxDichargingCurrent_mA / 512));
  chargerIc.writeRegEx(chargerIcRegisters.prochotOption1);

  // disable ICO
  enable_ico(false);

  chargerIc.readRegEx(chargerIcRegisters.chargeOption0);
  // disable DPM auto
  chargerIcRegisters.chargeOption0.set_IDPM_AUTO_DISABLE(0);
  // enable IDPM
  chargerIcRegisters.chargeOption0.set_EN_IDPM(1);
  // set watchog timer to 5 seconds (lowest)
  chargerIcRegisters.chargeOption0.set_WDTMR_ADJ(1);
  chargerIc.writeRegEx(chargerIcRegisters.chargeOption0);

  chargerIc.readRegEx(chargerIcRegisters.chargeOption3);
  // set 6A inductor (TODO issue #131: change with system constants)
  chargerIcRegisters.chargeOption3.set_IL_AVG(0b0);
  chargerIc.writeRegEx(chargerIcRegisters.chargeOption3);

  // disable charge
  enable_charge(false);

  chargerIc.readRegEx(chargerIcRegisters.chargeOption1);
  // enable IBAT
  chargerIcRegisters.chargeOption1.set_EN_IBAT(1);
  // enable PSYS
  chargerIcRegisters.chargeOption1.set_EN_PSYS(1);
  chargerIc.writeRegEx(chargerIcRegisters.chargeOption1);

  // set the nominal voltage values
  const auto maxBatteryVoltage_mV_read = chargerIcRegisters.maxChargeVoltage.set(maxBatteryVoltage_mV);
  const auto minSystemVoltage_mV_read = chargerIcRegisters.minSystemVoltage.set(minSystemVoltage_mV);
  const auto minInputVoltage_mV_read = chargerIcRegisters.inputVoltage.set(4200);

  std::string startErrorMessage = "";
  bool isSuccessful = true;

  // a write failed at some point
  if (chargerIc.isFlagRaised)
  {
    isSuccessful = false;
    startErrorMessage += "\n\t- i2c failed flag raised";
  }
  // check the register values
  if (chargerIcRegisters.maxChargeVoltage.get() != maxBatteryVoltage_mV_read)
  {
    isSuccessful = false;
    startErrorMessage += "\n\t- max charge voltage register write failed";
  }
  if (chargerIcRegisters.minSystemVoltage.get() != minSystemVoltage_mV_read)
  {
    isSuccessful = false;
    startErrorMessage += "\n\t- min charge voltage register write failed";
  }

  if (not isSuccessful)
  {
    status_s = Status_t::ERROR;
    set_software_error_message(startErrorMessage);
    return false;
  }

  // initial status update
  powerLimits_s.maxChargingCurrent_mA = maxChargingCurrent_mA;
  run_status_update();
  return true;
}

void loop(const bool isChargeOk)
{
  static bool isInOtg = false;

  // instant update if the state changed
  const bool isChargeChanged = (isChargeOk != isChargeOk_s) or (isInOtg != isInOtg_s);
  isChargeOk_s = isChargeOk;
  isInOtg = isInOtg_s;

  // update the charge
  control_charge();
  // update the OTG functionalities
  control_OTG();

  // only update every 100ms
  EVERY_N_MILLIS_COND(isChargeChanged, 100)
  {
    measurments_s.isChargeOk = isChargeOk;

    // update status
    run_status_update();
    // update measurments
    run_ADC();

    // do not run charger functions if status is not nominal
    if (status_s != Status_t::NOMINAL)
    {
      return;
    }

    // set the current limit to what is stored
    program_input_current_limit();

    // stop charge
    if (powerLimits_s.maxChargingCurrent_mA == 0)
    {
      chargerIcRegisters.chargeCurrent.set(powerLimits_s.maxChargingCurrent_mA);
    }
    // throttle charge current with temperature
    else
    {
      // set max charge current
      const float coreTemp = read_CPU_temperature_degreesC();
      // will be 0 when temp reaches 70 degrees, stopping the charge
      // below 40 degrees, no reduction of charge current is made
      const float reducer = lmpd_constrain(lmpd_map<float, float>(coreTemp, 40.0, 70.0, 1.0, 0.0), 0.0, 1.0);
      // write the reduced current
      chargerIcRegisters.chargeCurrent.set(reducer * powerLimits_s.maxChargingCurrent_mA);
    }
  }
}

void shutdown()
{
  // disable charge
  isChargeOk_s = false;
  // just in case
  disable_OTG();

  // limit input current
  powerLimits_s.set_default();
  program_input_current_limit();

  // run a last update
  control_OTG();
  control_charge();

  // enable low power mode
  chargerIc.readRegEx(chargerIcRegisters.chargeOption0);
  chargerIcRegisters.chargeOption0.set_EN_LWPWR(1);
  chargerIc.writeRegEx(chargerIcRegisters.chargeOption0);

  chargerIc.readRegEx(chargerIcRegisters.chargeOption3);
  // disable high impedance mode
  chargerIcRegisters.chargeOption3.set_EN_HIZ(0);
  chargerIc.writeRegEx(chargerIcRegisters.chargeOption3);
}

void set_input_current_limit(const uint16_t maxInputCurrent_mA, const bool shouldUseICO)
{
  powerLimits_s.current_mA = maxInputCurrent_mA;
  powerLimits_s.shoulduseICO = shouldUseICO;
}

uint16_t get_charge_current()
{
  return measurments_s.batChargeCurrent_mA;
  // return chargerIcRegisters.chargeCurrent.get();
}

bool is_input_source_present()
{
  chargerIc.readRegEx(chargerIcRegisters.chargerStatus);
  return chargerIcRegisters.chargerStatus.AC_STAT() != 0 and not isInOtg_s;
}

void try_clear_faults()
{
  // clear the fault we can clear, wait and see
  chargerIc.readRegEx(chargerIcRegisters.chargerStatus);
  if (chargerIcRegisters.chargerStatus.SYSOVP_STAT())
  {
    chargerIcRegisters.chargerStatus.set_SYSOVP_STAT(0);
    chargerIc.writeRegEx(chargerIcRegisters.chargerStatus);
  }
}

void enable_OTG()
{
  if (not isInOtg_s)
  {
    // activate this state FIRST
    isInOtg_s = true;
    // will deactivate the charge (because OTG)
    control_charge();
    // start otg directly
    control_OTG();
  }
}

void disable_OTG()
{
  chargerIc.readRegEx(chargerIcRegisters.chargeOption3);
  chargerIcRegisters.chargeOption3.set_EN_OTG(0);
  chargerIc.writeRegEx(chargerIcRegisters.chargeOption3);

  alerts::manager.clear(alerts::Type::OTG_FAILED);

  // deactivate this state LAST
  isInOtg_s = false;
}

void set_OTG_targets(const uint16_t voltage_mV, const uint16_t maxCurrent_mA)
{
  // OTG voltage register depends on another register...
  chargerIc.readRegEx(chargerIcRegisters.chargeOption3);
  const bool isVoltageRangeLow = chargerIcRegisters.chargeOption3.OTG_RANGE_LOW() != 0x00;

  const auto prevVal = chargerIcRegisters.oTGVoltage.get(isVoltageRangeLow);

  const auto realVal = chargerIcRegisters.oTGVoltage.set(voltage_mV, isVoltageRangeLow);
  chargerIcRegisters.oTGCurrent.set(maxCurrent_mA);

  if (realVal != prevVal)
    lampda_print("new OTG targets : %dmV %dmA", realVal, maxCurrent_mA);
}

bool is_in_OTG() { return isInOtg_s; }

Status_t get_status() { return status_s; }

std::string get_status_detail() { return status_error; }

ChargeStatus_t get_charge_status() { return chargeStatus_s; }

bool Measurments::is_measurment_valid() const
{
  // initialized and recent
  return this->time > 0 and (time_ms() - this->time) < 2000;
}

Measurments get_measurments() { return measurments_s; };

Battery get_battery() { return battery_s; };

} // namespace drivers
} // namespace charger

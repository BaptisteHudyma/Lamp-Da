#include "charging_ic.h"

#include <cstdint>

#include "BQ25703A.h"
#include "src/system/alerts.h"
#include "src/system/utils/print.h"

#include "src/system/platform/time.h"
#include "src/system/platform/gpio.h"

namespace BQ25703A {

// Initialise the device and library
bq2573a::BQ25703A chargerIc;

// Create instance of registers data structure
bq2573a::BQ25703A::Regt BQ25703Areg;

// store the ADC measurments
static Measurments measurments_s;
// store the battery measurments
static Battery battery_s;

// store the component status
inline static Status_t status_s = Status_t::UNINITIALIZED;
// store the current charge status of the system
inline static ChargeStatus_t chargeStatus_s = ChargeStatus_t::OFF;

// charger can charge if needed
inline static bool isChargeOk_s = false;
// charger is charging
inline static bool isChargeEnabled_s = false;
// charger is in OTG state
inline static bool isInOtg_s = false;

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
  if (BQ25703Areg.chargerStatus.Fault_ACOV() or BQ25703Areg.chargerStatus.Fault_BATOC() or
      BQ25703Areg.chargerStatus.Fault_ACOC() or BQ25703Areg.chargerStatus.SYSOVP_STAT() or
      BQ25703Areg.chargerStatus.Fault_Latchoff() or BQ25703Areg.chargerStatus.Fault_OTG_OVP() or
      BQ25703Areg.chargerStatus.Fault_OTG_UCP())
  {
    lampda_print("Charger ic faults: %d%d%d%d%d%d%d",
                 BQ25703Areg.chargerStatus.Fault_ACOV(),
                 BQ25703Areg.chargerStatus.Fault_BATOC(),
                 BQ25703Areg.chargerStatus.Fault_ACOC(),
                 BQ25703Areg.chargerStatus.SYSOVP_STAT(),
                 BQ25703Areg.chargerStatus.Fault_Latchoff(),
                 BQ25703Areg.chargerStatus.Fault_OTG_OVP(),
                 BQ25703Areg.chargerStatus.Fault_OTG_UCP());
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
  const byte initialReg0read = BQ25703Areg.chargerStatus.val0;
  const byte initialReg1read = BQ25703Areg.chargerStatus.val1;

  chargerIc.readRegEx(BQ25703Areg.chargerStatus);

  if (initialReg0read != BQ25703Areg.chargerStatus.val0 or initialReg1read != BQ25703Areg.chargerStatus.val1)
  {
    // check faults
    run_fault_detection();
  }

  // handle OTG alerts
  if (isInOtg_s)
  {
    if (BQ25703Areg.chargerStatus.IN_OTG())
    // TODO: check that the EN_OTG is still enabled
    // if (BQ25703Areg.chargeOption3.EN_OTG())
    {
      alerts::manager.clear(alerts::Type::OTG_FAILED);
      alerts::manager.raise(alerts::Type::OTG_ACTIVATED);
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
  chargerIc.readRegEx(BQ25703Areg.chargeOption0);
  const int shouldInihibit = shouldCharge ? 0 : 1;

  // if charge status changed, writte it
  if (BQ25703Areg.chargeOption0.CHRG_INHIBIT() != shouldInihibit)
  {
    BQ25703Areg.chargeOption0.set_CHRG_INHIBIT(shouldInihibit);
    chargerIc.writeRegEx(BQ25703Areg.chargeOption0);
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

      DigitalPin(DigitalPin::GPIO::otgSignal).set_high(true);
      delay_ms(1);

      chargerIc.readRegEx(BQ25703Areg.chargeOption3);
      BQ25703Areg.chargeOption3.set_EN_OTG(1);
      chargerIc.writeRegEx(BQ25703Areg.chargeOption3);

      // alert will be lowered on time
      alerts::manager.clear(alerts::Type::OTG_ACTIVATED);
      alerts::manager.raise(alerts::Type::OTG_FAILED);

      delay_ms(10);
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

      if (measurment.vbus_mV < 3200)
      {
        // wait for voltage to climb on VBUS
        return;
      }
      if (measurment.vbus_mA > 0)
      {
        // update the last used time
        lastOTGUsedTime_ms = time_ms();
      }

      // if some time has passed since the last use
      if (time_ms() - lastOTGUsedTime_ms > 10000)
      {
        // disable the OTG if it's not used
        disable_OTG();
      }
      else
      {
        // update the in OTG status to avoid deconnection
        DigitalPin(DigitalPin::GPIO::otgSignal).set_high(true);
        chargerIc.readRegEx(BQ25703Areg.chargeOption3);
        BQ25703Areg.chargeOption3.set_EN_OTG(1);
        chargerIc.writeRegEx(BQ25703Areg.chargeOption3);
      }
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

      DigitalPin(DigitalPin::GPIO::otgSignal).set_high(false);
      delay_ms(1);
      // 5V 0A for OTG (default)
      set_OTG_targets(5000, 0);

      alerts::manager.clear(alerts::Type::OTG_ACTIVATED);
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
    chargerIc.readRegEx(BQ25703Areg.chargeOption3);
    // Do not reset ICO, if it is already enabled.
    if (BQ25703Areg.chargeOption3.EN_ICO_MODE() != 0)
      return;

    // enable ICO
    BQ25703Areg.chargeOption3.set_EN_ICO_MODE(1);
    BQ25703Areg.chargeOption3.set_RESET_VINDPM(1);
    chargerIc.writeRegEx(BQ25703Areg.chargeOption3);
  }
  else
  {
    // disable ICO
    BQ25703Areg.chargeOption3.set_EN_ICO_MODE(0);
    chargerIc.writeRegEx(BQ25703Areg.chargeOption3);
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

  // output voltage saturated, battery is not here
  battery_s.isPresent = battery_s.voltage_mV < BQ25703Areg.maxChargeVoltage.get();

  // check the charging status
  if (not isInputSourcePresent or chargingCurrent <= 0)
  {
    chargeStatus_s = ChargeStatus_t::OFF;
  }
  //
  else if (chargingCurrent <= powerLimits_s.maxChargingCurrent_mA * 0.1)
  {
    chargeStatus_s = ChargeStatus_t::SLOW_CHARGE;
  }
  else if (BQ25703Areg.chargerStatus.IN_PCHRG() != 0)
  {
    chargeStatus_s = ChargeStatus_t::PRECHARGE;
  }
  else if (BQ25703Areg.chargerStatus.IN_FCHRG() != 0)
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
    chargerIc.readRegEx(BQ25703Areg.aDCOption);
    // start a new ADC read
    BQ25703Areg.aDCOption.set_ADC_CONV(0);
    BQ25703Areg.aDCOption.set_ADC_START(1);
    // Set ADC for each parameters
    BQ25703Areg.aDCOption.set_EN_ADC_CMPIN(1);
    BQ25703Areg.aDCOption.set_EN_ADC_VBUS(1);
    BQ25703Areg.aDCOption.set_EN_ADC_PSYS(1);
    BQ25703Areg.aDCOption.set_EN_ADC_IIN(1);
    BQ25703Areg.aDCOption.set_EN_ADC_IDCHG(1);
    BQ25703Areg.aDCOption.set_EN_ADC_ICHG(1);
    BQ25703Areg.aDCOption.set_EN_ADC_VSYS(1);
    BQ25703Areg.aDCOption.set_EN_ADC_VBAT(1);
    // write the register
    chargerIc.writeRegEx(BQ25703Areg.aDCOption);

    isAdcTriggered = true;
  }
  else
  {
    // check if the measurment is made
    chargerIc.readRegEx(BQ25703Areg.aDCOption);
    if (BQ25703Areg.aDCOption.ADC_START() != 0)
    {
      // ADC in progress
      return;
    }

    // reset the flag to trigger a new read
    isAdcTriggered = false;

    // store everything in the measurment struct
    measurments_s.time = time_ms();
    measurments_s.vbus_mV = BQ25703Areg.aDCVBUSPSYS.get_VBUS();
    measurments_s.psys_mV = BQ25703Areg.aDCVBUSPSYS.get_PSYS();
    measurments_s.batChargeCurrent_mA = BQ25703Areg.aDCIBAT.get_ICHG();
    measurments_s.batDischargeCurrent_mA = BQ25703Areg.aDCIBAT.get_IDCHG();
    measurments_s.vbus_mA = BQ25703Areg.aDCIINCMPIN.get_IIN();
    measurments_s.cmpin_mA = BQ25703Areg.aDCIINCMPIN.get_CMPIN();
    measurments_s.vsys_mV = BQ25703Areg.aDCVSYSVBAT.get_VSYS();
    measurments_s.battery_mV = BQ25703Areg.aDCVSYSVBAT.get_VBAT();

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
  BQ25703Areg.chargeOption0.set_EN_IDPM(1);
  chargerIc.writeRegEx(BQ25703Areg.chargeOption0);

  if (isChargeOk_s and inputCurrentLimit_mA > 0)
  {
    enable_ico(shouldUseICO);

    const uint16_t writtenCurrent_mA = BQ25703Areg.iIN_HOST.set(inputCurrentLimit_mA);

    // check that the register is set
    if (writtenCurrent_mA != BQ25703Areg.iIN_HOST.get() or
        (not shouldUseICO and writtenCurrent_mA != BQ25703Areg.iIN_DPM.get()))
    {
      // read/write error
      /*Serial.print(writtenCurrent_mA);
      Serial.print("mA != ");
      Serial.print(BQ25703Areg.iIN_HOST.get());
      Serial.print("mA OR ");
      Serial.print(not shouldUseICO);
      Serial.print(" and ");
      Serial.print(writtenCurrent_mA);
      Serial.print("mA != ");
      Serial.println(BQ25703Areg.iIN_DPM.get());
      */
      status_s = Status_t::ERROR;
      return;
    }

    // force disable ILIM pin hardware input current limit
    BQ25703Areg.chargeOption2.set_EN_EXTILIM(0);
    chargerIc.writeRegEx(BQ25703Areg.chargeOption2);
  }
  // set in current to 0
  else
  {
    // disable ico
    enable_ico(false);

    // input current to 0
    BQ25703Areg.iIN_HOST.set(0);

    // enable hardware limit check
    BQ25703Areg.chargeOption2.set_EN_EXTILIM(1);
    chargerIc.writeRegEx(BQ25703Areg.chargeOption2);
  }

  // read IDPM status (should be enabled)
  chargerIc.readRegEx(BQ25703Areg.chargeOption0);
  if (BQ25703Areg.chargeOption0.EN_IDPM() == 0)
  {
    lampda_print("EN IDPM not enabled");
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
            const uint16_t minInputVoltage_mV,
            const uint16_t maxChargingCurrent_mA,
            const bool forceReset)
{
  if (chargerIc.isFlagRaised or BQ25703Areg.manufacturerID.get_manufacturerID() != bq2573a::MANUFACTURER_ID or
      BQ25703Areg.deviceID.get_deviceID() != bq2573a::DEVICE_ID)
  {
    // error: not detected, or those constants do not indicate a BQ25703A
    // charger
    status_s = Status_t::ERROR_COMPONENT;
    return false;
  }

  // reset the parameters if needed
  if (forceReset)
  {
    // write the reset flag
    chargerIc.readRegEx(BQ25703Areg.chargeOption3);
    BQ25703Areg.chargeOption3.set_RESET_REG(1);
    chargerIc.writeRegEx(BQ25703Areg.chargeOption3);

    // wait until the flag is lowered
    do
    {
      delay_ms(5);
      chargerIc.readRegEx(BQ25703Areg.chargeOption3);
    } while (BQ25703Areg.chargeOption3.RESET_REG() == 1);
  }

  // everything went fine (for now)

  // disable the OTG (safety)
  disable_OTG();

  status_s = Status_t::NOMINAL;

  chargerIc.readRegEx(BQ25703Areg.chargeOption3);
  // disable high impedance mode
  BQ25703Areg.chargeOption3.set_EN_HIZ(0);
  chargerIc.writeRegEx(BQ25703Areg.chargeOption3);

  // disable low power mode
  BQ25703Areg.chargeOption0.set_EN_LWPWR(0);
  chargerIc.writeRegEx(BQ25703Areg.chargeOption0);

  BQ25703Areg.prochotOption1.set_IDCHG_VTH(16384 / 512);
  chargerIc.writeRegEx(BQ25703Areg.prochotOption1);

  // disable ICO
  enable_ico(false);

  chargerIc.readRegEx(BQ25703Areg.chargeOption0);
  // disable DPM auto
  BQ25703Areg.chargeOption0.set_IDPM_AUTO_DISABLE(0);
  // enable IDPM
  BQ25703Areg.chargeOption0.set_EN_IDPM(1);
  // set watchog timer to 5 seconds (lowest)
  BQ25703Areg.chargeOption0.set_WDTMR_ADJ(1);
  chargerIc.writeRegEx(BQ25703Areg.chargeOption0);

  // disable charge
  enable_charge(false);

  chargerIc.readRegEx(BQ25703Areg.chargeOption1);
  // enable IBAT
  BQ25703Areg.chargeOption1.set_EN_IBAT(1);
  // enable PSYS
  BQ25703Areg.chargeOption1.set_EN_PSYS(1);
  chargerIc.writeRegEx(BQ25703Areg.chargeOption1);

  // set the nominal voltage values
  const auto maxBatteryVoltage_mV_read = BQ25703Areg.maxChargeVoltage.set(maxBatteryVoltage_mV);
  const auto minSystemVoltage_mV_read = BQ25703Areg.minSystemVoltage.set(minSystemVoltage_mV);
  const auto minInputVoltage_mV_read = BQ25703Areg.inputVoltage.set(minInputVoltage_mV);

  // a write failed at some point
  if (chargerIc.isFlagRaised)
  {
    status_s = Status_t::ERROR;
    return false;
  }
  // check the register values
  if (BQ25703Areg.maxChargeVoltage.get() != maxBatteryVoltage_mV_read)
  {
    status_s = Status_t::ERROR;
    return false;
  }
  if (BQ25703Areg.minSystemVoltage.get() != minSystemVoltage_mV_read)
  {
    status_s = Status_t::ERROR;
    return false;
  }
  /*
  // there is a problem with this register on all the IC I tester
  if (BQ25703Areg.inputVoltage.get() != minInputVoltage_mV_read) {
    status_s = Status_t::ERROR;
    return false;
  }
  */

  // initial status update
  powerLimits_s.maxChargingCurrent_mA = maxChargingCurrent_mA;
  run_status_update();
  return true;
}

void loop(const bool isChargeOk)
{
  const bool isChargeChanged = isChargeOk != isChargeOk_s;
  isChargeOk_s = isChargeOk;

  static uint32_t lastUpdateTime = 0;

  // update the charge
  control_charge();
  // update the OTG functionalities
  control_OTG();

  const uint32_t time = time_ms();
  // only update every 100ms
  if (isChargeChanged or lastUpdateTime == 0 or time - lastUpdateTime >= 100)
  {
    lastUpdateTime = time;

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

    // set max charge current
    BQ25703Areg.chargeCurrent.set(powerLimits_s.maxChargingCurrent_mA);
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
  chargerIc.readRegEx(BQ25703Areg.chargeOption0);
  BQ25703Areg.chargeOption0.set_EN_LWPWR(1);
  chargerIc.writeRegEx(BQ25703Areg.chargeOption0);
}

void set_input_current_limit(const uint16_t maxInputCurrent_mA, const bool shouldUseICO)
{
  powerLimits_s.current_mA = maxInputCurrent_mA;
  powerLimits_s.shoulduseICO = shouldUseICO;
}

bool is_input_source_present() { return BQ25703Areg.chargerStatus.AC_STAT() != 0; }

void try_clear_faults()
{
  // clear the fault we can clear, wait and see
  chargerIc.readRegEx(BQ25703Areg.chargerStatus);
  if (BQ25703Areg.chargerStatus.SYSOVP_STAT())
  {
    BQ25703Areg.chargerStatus.set_SYSOVP_STAT(0);
    chargerIc.writeRegEx(BQ25703Areg.chargerStatus);
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
  }
}

void disable_OTG()
{
  chargerIc.readRegEx(BQ25703Areg.chargeOption3);
  BQ25703Areg.chargeOption3.set_EN_OTG(0);
  chargerIc.writeRegEx(BQ25703Areg.chargeOption3);

  alerts::manager.clear(alerts::Type::OTG_ACTIVATED);
  alerts::manager.clear(alerts::Type::OTG_FAILED);

  // deactivate this state LAST
  isInOtg_s = false;
}

void set_OTG_targets(const uint16_t voltage_mV, const uint16_t maxCurrent_mA)
{
  BQ25703Areg.oTGVoltage.set(voltage_mV);
  BQ25703Areg.oTGCurrent.set(maxCurrent_mA);
}

Status_t get_status() { return status_s; }

ChargeStatus_t get_charge_status() { return chargeStatus_s; }

bool Measurments::is_measurment_valid() const
{
  // initialized and recent
  return this->time > 0 and (time_ms() - this->time) < 2000;
}

Measurments get_measurments() { return measurments_s; };

Battery get_battery() { return battery_s; };

} // namespace BQ25703A
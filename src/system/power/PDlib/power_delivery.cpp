#include "power_delivery.h"

#include "drivers/usb_pd_driver.h"
#include "drivers/tcpm_driver.h"
#include "usb_pd.h"

#include "src/system/utils/time_utils.h"

#include "src/system/physical/battery.h"

#include "src/system/platform/print.h"
#include "src/system/platform/time.h"
#include "src/system/platform/gpio.h"
#include "src/system/platform/threads.h"

#include "src/system/power/charging_ic.h"
#include "src/system/power/power_gates.h"

// remove this to remove all PD algorithms
#define USE_PD_ALGO_LOOP

// we only have one device, so always index 0
static constexpr int devicePort = 0;
// USB-C Specific - TCPM start 1
const struct tcpc_config_t tcpc_config = {
        devicePort, fusb302_I2C_SLAVE_ADDR, &fusb302_tcpm_drv, tcpc_alert_polarity::TCPC_ALERT_ACTIVE_HIGH};
// USB-C Specific - TCPM end 1

static bool canUseSourcePower_s = false;

namespace powerDelivery {

// set to true when a power source is detected
inline static bool isPowerSourceDetected_s = false;
// set to the time when the source is detected
inline static uint32_t powerSourceDetectedTime_s = 0;

inline static bool should_run_pd_state_machine = true;
void suspend_pd_state_machine()
{
  if (should_run_pd_state_machine)
    supsend_usb_pd(1);
  should_run_pd_state_machine = false;
}
void resume_pd_state_machine()
{
  if (!should_run_pd_state_machine)
    supsend_usb_pd(0);
  should_run_pd_state_machine = true;
}

int get_vbus_voltage()
{
  static int vbusVoltage = 0;
  // do not spam the system
  EVERY_N_MILLIS(100) { vbusVoltage = tcpm_get_vbus_voltage(); }
  return vbusVoltage;
}

// check if the charger can use the max power
bool can_use_PD_full_power()
{
  // voltage on VBUS is greater than (negociated voltage minus a threshold)
  return get_available_pd_voltage_mV() > 0 and
         // the algo confirms that we are a sink
         is_sink_ready() and
         // the voltage/current rise should be over
         get_vbus_voltage() >= get_available_pd_voltage_mV() - 1000;
}

uint32_t lastPDdetected = 0;
bool is_usb_pd()
{
  const bool isPd = is_pd_conector();
  if (isPd)
  {
    lastPDdetected = time_ms();
  }
  return isPd;
}

bool is_cable_detected()
{
  enum tcpc_cc_voltage_status cc1;
  enum tcpc_cc_voltage_status cc2;
  tcpm_get_cc(&cc1, &cc2);
  return cc1 != TYPEC_CC_VOLT_OPEN or cc2 != TYPEC_CC_VOLT_OPEN;
}

// check if the source is simple USB, with a stabilize delay
bool is_standard_port()
{
  return isPowerSourceDetected_s and
         // let some time pass after a new detection
         time_ms() - powerSourceDetectedTime_s > 1500 and
         // last pd detected was some time ago
         time_ms() - lastPDdetected > 1000;
}

void ic_interrupt()
{
  // wake up interrupt thread (cannot run code in the interrupt callback)
  notify_thread(pdInterruptHandle_taskName, 2);
}

bool is_vbus_powered()
{
  // more reliable when vbus is under load from battery charging
  const auto& measurments = charger::drivers::get_measurments();
  if (measurments.isChargeOk and measurments.vbus_mA >= 100)
    return get_vbus_voltage() >= 3000;
  else
    // no load, threshold is higher
    return get_vbus_voltage() >= 4300;
}

struct UsbPDData
{
  bool isVbusPowered;
  bool isPowerSourceDetected;
  bool isUsbPd;
  int vbusVoltage;

  uint32_t maxInputCurrent;
  uint32_t maxInputVoltage;

  std::string pdAlgoStatus;

  // when true, this struct has changed !
  bool hasChanged = false;

  void update()
  {
    const bool newisVbusPowered = is_vbus_powered();
    if (newisVbusPowered != isVbusPowered)
    {
      hasChanged = true;
      isVbusPowered = newisVbusPowered;
    }

    const bool cc = isPowerSourceDetected_s;
    if (cc != isPowerSourceDetected)
    {
      hasChanged = true;
      isPowerSourceDetected = isPowerSourceDetected_s;
    }

    const bool newisUsbPd = is_usb_pd();
    if (newisUsbPd != isUsbPd)
    {
      hasChanged = true;
      isUsbPd = newisUsbPd;
    }

    const auto newvbusVoltage = get_vbus_voltage();
    if (newvbusVoltage != vbusVoltage)
    {
      // hasChanged = true;
      vbusVoltage = newvbusVoltage;
    }

    const auto newmaxInputCurrent = get_available_pd_current_mA();
    if (newmaxInputCurrent != maxInputCurrent)
    {
      hasChanged = true;
      maxInputCurrent = newmaxInputCurrent;
    }

    const auto newmaxInputVoltage = get_available_pd_voltage_mV();
    if (newmaxInputVoltage != maxInputVoltage)
    {
      hasChanged = true;
      maxInputVoltage = newmaxInputVoltage;
    }

    const auto& newStatus = std::string(get_state_cstr());
    if (newStatus != pdAlgoStatus)
    {
      hasChanged = true;
      pdAlgoStatus = newStatus;
    }

    if (hasChanged)
    {
      serial_show();
    }
  }

  void serial_show()
  {
    lampda_print("PD algo: %d %d%d%d: [PDO %.2fV %.2fA] %.2fV | %s",
                 should_run_pd_state_machine,
                 isVbusPowered,
                 isPowerSourceDetected,
                 isUsbPd,
                 maxInputVoltage / 1000.0,
                 maxInputCurrent / 1000.0,
                 vbusVoltage / 1000.0,
                 pdAlgoStatus.c_str());
    hasChanged = false;
    lampda_print(
            "allowed usage: %d mA, %d mV", get_allowed_consuption().current_mA, get_allowed_consuption().voltage_mV);
  }
};
UsbPDData data;

void show_pd_status() { data.serial_show(); }

/**
 *
 *      HEADER FUNCTIONS BELOW
 *
 */

void interrupt_handle()
{
  // this thread only runs when signal is sent
  wait_notification(0);

#ifdef USE_PD_ALGO_LOOP
  // only waken up on thread update
  tcpc_alert();
#endif
}

void pd_run()
{
  // PD loop limits the run of this threads by waiting for events
  // DO NOT REMOVE THE PD STATE MACHINE
  // or -> add a delay to this loop instead
#ifdef USE_PD_ALGO_LOOP
  pd_loop();
#else
  delay_ms(10);
#endif

  // partner asked us to stop to pull current
  if (should_stop_vbus_charge())
  {
    powergates::disable_gates();
    return;
  }
#if 0
  static bool isFastRoleSwap = false;
  // ignore source activity if we are otg (prevent spurious reset)
  if (is_switching_to_otg())
  {

    if (not isFastRoleSwap)
    {
      isFastRoleSwap = true;

      // prepare fast role swap
      lampda_print("prepare fast role swap");
      powergates::disable_gates();

      // force otg on, and prep vbus gate, all in this loop iteration (skip all safety steps !!!)
      charger::drivers::set_OTG_targets(5000, 1000);
      charger::drivers::enable_OTG();

      DigitalPin dischargeVbus(DigitalPin::GPIO::Output_DischargeVbus);
      DigitalPin fastRoleSwap(DigitalPin::GPIO::Output_VbusFastRoleSwap);
      dischargeVbus.set_high(true);
      fastRoleSwap.set_high(true);

      // wait until OTG drops to acceptable level
      uint64_t timeout = get_time().val + 500 * MSEC_US;
      while (get_time().val < timeout and charger::drivers::get_measurments().vbus_mV > 5500)
      {
        delay_us(50);
      }

      // wait for vbus voltage to drop to acceptable level
      timeout = get_time().val + 500 * MSEC_US;
      while (get_time().val < timeout and tcpm_get_vbus_voltage() > 5500)
      {
        delay_us(50);
      }

      // enable gate direction
      DigitalPin(DigitalPin::GPIO::Output_VbusDirection).set_high(true);
      dischargeVbus.set_high(false);
      fastRoleSwap.set_high(false);
      powergates::enable_vbus_gate_DIRECT();
    }
    // after the activation turn, disable the flag
    else
      isPowerSourceDetected_s = false;

    return;
  }
  else if (isFastRoleSwap)
  {
    charger::drivers::disable_OTG();

    // enable gate direction
    DigitalPin(DigitalPin::GPIO::Output_VbusDirection).set_high(false);
    powergates::disable_gates();
  }
  isFastRoleSwap = false;
#endif
}

static bool isSetup = false;

bool setup()
{
  // 0 is success
  if (i2c_check_existence(devicePort, fusb302_I2C_SLAVE_ADDR) != 0)
  {
    return false;
  }

  pd_init();
  delay_ms(5);
  pd_startup();

  DigitalPin chargerPin(DigitalPin::GPIO::Signal_PowerDelivery);
  chargerPin.attach_callback(ic_interrupt,
                             DigitalPin::Interrupt::kFallingEdge); // normal high, so focus on falling edge

  isSetup = true;
  return true;
}

void start_threads()
{
  if (!isSetup)
    return;

  // start task scheduler, in suspended state
  start_thread(task_scheduler, taskScheduler_taskName, 2, 255);
  // start interrupt handle, in suspended state
  start_thread(interrupt_handle, pdInterruptHandle_taskName, 2, 255);
  // start pd handle loop
  start_thread(pd_run, pd_taskName, 1, 1024);
}

void loop()
{
  data.update();

  // update battery level
  set_battery_level(static_cast<uint8_t>(battery::get_battery_level() / 100));

  // ignore source activity if we are otg (prevent spurious reset)
  if (is_switching_to_otg())
  {
    return;
  }

  static uint32_t lastVbusValid = 0;

  // source detected
  const uint32_t time = time_ms();
  if (is_vbus_powered())
  {
    if (not isPowerSourceDetected_s)
    {
      powerSourceDetectedTime_s = time;
      isPowerSourceDetected_s = true;
    }
    lastVbusValid = time;
  }
  // deglitch time
  else if (isPowerSourceDetected_s and
           // plugged in power can flicker
           time - powerSourceDetectedTime_s > 100 and
           // vbus can become briefly invalid when using the ICO algorithm
           time - lastVbusValid > 1000)
  {
    isPowerSourceDetected_s = false;
    reset_cache();
  }

  if (
          // valid input source
          isPowerSourceDetected_s and (
                                              // is pd with power
                                              (is_usb_pd() and can_use_PD_full_power()) or
                                              // is standard usb c
                                              (is_standard_port())))
  {
    canUseSourcePower_s = true;
  }
  else
  {
    // no deglitch, this needs to be reactive
    canUseSourcePower_s = false;
  }
}

void shutdown() { pd_turn_off(); }

uint16_t get_max_input_current()
{
  // safety
  if (!isPowerSourceDetected_s)
    return 0;

  // power delivery detected
  if (is_usb_pd())
  {
    if (can_use_PD_full_power())
    {
      // do not use the whole current capabilities, or the source will cut us off
      return static_cast<uint16_t>(get_available_pd_current_mA() * 0.90);
    }
  }
  // no usb pd since some time, and vbus seems stable so try to use it
  else if (is_standard_port())
  {
    // maximum USB current (hacky, would prefer USB type detection)
    return 800;
  }
  // we dont known for now the type of connection
  return 0;
}

bool is_power_available() { return isPowerSourceDetected_s; }

bool can_use_power() { return canUseSourcePower_s; }

void force_set_to_source_mode(const bool force) { force_set_to_source(force ? 1 : 0); }

OTGParameters get_otg_parameters()
{
  const auto& otg = get_OTG_requested_parameters();
  OTGParameters tmp;
  tmp.requestedCurrent_mA = otg.requestedCurrent_mA;
  tmp.requestedVoltage_mV = otg.requestedVoltage_mV;
  return tmp;
}

void allow_otg(const bool allow) { set_allow_power_sourcing(allow); }

bool is_switching_to_otg() { return is_activating_otg() != 0; }

std::vector<PDOTypes> get_available_pd()
{
  std::vector<PDOTypes> pdos;
  for (uint8_t i = 0; i < get_pd_source_cnt(); ++i)
  {
    PDOTypes t;
    pd_extract_pdo_power(get_pd_source(i), &t.maxCurrent_mA, &t.voltage_mv);
    pdos.emplace_back(t);
  }

  return pdos;
}

} // namespace powerDelivery

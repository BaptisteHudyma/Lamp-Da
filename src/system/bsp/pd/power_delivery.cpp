#include "power_delivery.h"

#include <etl/string.h>

#include "src/system/driver/pd/usb_pd_driver.h"
#include "src/system/driver/pd/tcpm_driver.h"

#include "usb_pd.h"
#include "input_power_policy.h"

#include "src/system/utils/time_utils.h"

#include "src/system/component/battery.h"

#include "src/system/hal/time.h"
#include "src/system/hal/gpio.h"

#include "src/system/bsp/charging_ic.h"
#include "src/system/bsp/power_gates.h"
#include "src/system/bsp/text_out.h"
#include "src/system/bsp/threads.h"

// TODO issue #132 remove when the mock components will be running
#ifdef LMBD_SIMULATION
#include "simulator/include/hardware_influencer.h"
#endif

// remove this to remove all PD algorithms
#define USE_PD_ALGO_LOOP

// we only have one device, so always index 0
static constexpr int devicePort = 0;
// USB-C Specific - TCPM start 1
const struct tcpc_config_t tcpc_config = {
        devicePort, fusb302_I2C_SLAVE_ADDR, &fusb302_tcpm_drv, tcpc_alert_polarity::TCPC_ALERT_ACTIVE_HIGH};
// USB-C Specific - TCPM end 1

static bool canUseSourcePower_s = false;

namespace lampda {
namespace bsp {
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
void allow_source_resets(const bool allow)
{
  // only on change: this restarts the negociation timers
  if (allow != (pd_is_reset_allowed() != 0))
    pd_set_reset_allowed(allow ? 1 : 0);
}

void resume_pd_state_machine()
{
  if (!should_run_pd_state_machine)
  {
    // the state machine restarts from a disconnected state: the source capabilities known before are not valid
    // anymore, and the PD negociation must be given time before falling back to the standard port current
    reset_source_capabilities();
    powerSourceDetectedTime_s = hal::time_ms();

    supsend_usb_pd(0);
  }
  should_run_pd_state_machine = true;
}

int get_vbus_voltage()
{
  static int vbusVoltage = 0;
  // do not spam the system
  EVERY_N_MILLIS(100)
  {
// TODO issue #132 remove when the mock components will be running
#ifdef LMBD_SIMULATION
    vbusVoltage = ::simulator::mock_electrical::inputVbusVoltage * 1000.0;
#else
    vbusVoltage = tcpm_get_vbus_voltage();
#endif
  }
  return vbusVoltage;
}

uint32_t lastPDdetected = 0;
bool is_usb_pd()
{
  const bool isPd = is_pd_conector();
  if (isPd)
  {
    lastPDdetected = hal::time_ms();
  }
  return isPd;
}

// snapshot of the source status, for the input power rules
policy::SourceStatus get_source_status()
{
  policy::SourceStatus status;
  // first, it updates the last PD detection time
  status.isUsbPd = is_usb_pd();

  const uint32_t time = hal::time_ms();
  status.isPowerSourceDetected = isPowerSourceDetected_s;
  status.timeSinceSourceDetected_ms = time - powerSourceDetectedTime_s;
  status.timeSinceLastPdDetected_ms = time - lastPDdetected;
  status.isSinkReady = is_sink_ready() != 0;
  status.isSinkNegotiating = is_sink_negotiating() != 0;
  status.negotiatedVoltage_mV = get_available_pd_voltage_mV();
  status.negotiatedCurrent_mA = get_available_pd_current_mA();
  const int vbus = get_vbus_voltage();
  status.vbusVoltage_mV = vbus > 0 ? static_cast<uint32_t>(vbus) : 0;
  return status;
}

// check if the charger can use the max power
bool can_use_PD_full_power() { return policy::can_use_pd_full_power(get_source_status()); }

// check if the source is simple USB, with a stabilize delay
bool is_standard_port() { return policy::is_standard_port(get_source_status()); }

void ic_interrupt()
{
  // wake up interrupt thread (cannot run code in the interrupt callback)
  bsp::threads::notify_thread(bsp::threads::pdInterruptHandle_taskName, 2);
}

static bool isSetup = false;

// debug counters of the interrupt servicing (see show_pd_status)
static uint32_t interruptStuckKicks_s = 0;
static uint32_t interruptAlertReruns_s = 0;

/**
 * The FUSB302 interrupt line is level triggered (active low, until all interrupt registers are read), but we only get
 * a callback on the falling edge. If an interrupt is raised before the callback is attached (VBUS and CC are already
 * there at boot, the CC lines have a permanent Rd), or while another one is pending, there is no new falling edge: the
 * line stays low forever, no message is received and no transmission completes (issue #633).
 * So service the interrupt on the line level.
 */
bool is_ic_interrupt_pending()
{
  static const hal::gpio::DigitalPin interruptPin(hal::gpio::DigitalPin::GPIO::Signal_PowerDelivery);
  return isSetup and not interruptPin.is_high();
}

bool is_vbus_powered()
{
  // more reliable when vbus is under load from battery charging
  const auto& measurments = bsp::charger::get_measurments();
  if (measurments.isChargeOk and measurments.vbus_mA >= 100)
    return get_vbus_voltage() >= 3000;
  else
    // no load, threshold is higher
    return get_vbus_voltage() >= 4300;
}

/**
 * \brief USB power delivery status storage
 */
struct UsbPDData
{
  /// True if some voltage is detected on VBUS
  bool isVbusPowered;
  /// True if the cable is plugged to a power source
  bool isPowerSourceDetected;
  /// True if the cable is identifies as PD compatible
  bool isUsbPd;
  /// Actual VBUS voltage, in millivolts
  int vbusVoltage;

  /// max current input voltage, in milliamps
  uint32_t maxInputCurrent;
  /// max input voltage, in millivolts
  uint32_t maxInputVoltage;

  /// Debug status for the algorithm
  etl::string<32> pdAlgoStatus;

  /// when true, this struct has changed !
  bool hasChanged = false;

  /// Update the values and update status
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

    const auto& newStatus = etl::string<32>(get_state_cstr());
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

  /// Debug to serial output
  void serial_show()
  {
    bsp::lampda_print("PD algo: %d %d%d%d: [PDO %.2fV %.2fA] %.2fV | %s",
                      should_run_pd_state_machine,
                      isVbusPowered,
                      isPowerSourceDetected,
                      isUsbPd,
                      maxInputVoltage / 1000.0,
                      maxInputCurrent / 1000.0,
                      vbusVoltage / 1000.0,
                      pdAlgoStatus.c_str());
    hasChanged = false;
    // the negociated contract (0 when there is none): published on PS_RDY, cleared on disconnection and hard reset
    struct pd_contract_info info;
    pd_get_contract_info(&info);
    bsp::lampda_print("PD contract: %s %lu mV, %lu mA",
                      info.is_explicit_contract ? "EXP" : "X",
                      static_cast<unsigned long>(get_available_pd_voltage_mV()),
                      static_cast<unsigned long>(get_available_pd_current_mA()));
  }
};
UsbPDData data;

const char* cc_level_str(const int level)
{
  switch (level)
  {
    case TYPEC_CC_VOLT_OPEN:
      return "open";
    case TYPEC_CC_VOLT_RA:
      return "Ra";
    case TYPEC_CC_VOLT_RD:
      return "Rd";
    case TYPEC_CC_VOLT_RP_DEF:
      return "Rp default";
    case TYPEC_CC_VOLT_RP_1_5:
      return "Rp 1.5A";
    case TYPEC_CC_VOLT_RP_3_0:
      return "Rp 3A";
    default:
      return "?";
  }
}

void show_pd_contract()
{
  struct pd_contract_info info;
  pd_get_contract_info(&info);

  bsp::lampda_print("Contract: %s, role %s, CC%d. Request %lumV %lumA",
                    info.is_explicit_contract ? "EXPLICIT" : "none",
                    info.power_role == PD_ROLE_SOURCE ? "source" : "sink",
                    info.polarity + 1,
                    static_cast<unsigned long>(info.requested_mv),
                    static_cast<unsigned long>(info.requested_ma));
  bsp::lampda_print(
          "Partner PD capable: %d, VBUS never low: %d, hard reset count: %d, negotiating: %d, resets "
          "allowed: %d",
          info.is_pd_capable,
          info.is_vbus_never_low,
          info.hard_reset_count,
          is_sink_negotiating(),
          pd_is_reset_allowed());

  // what the charger is allowed to use, and why
  const auto status = get_source_status();
  bsp::lampda_print("Input: published %lumV %lumA, VBUS %lumV. Detected %d since %lums, last PD %lums ago",
                    static_cast<unsigned long>(status.negotiatedVoltage_mV),
                    static_cast<unsigned long>(status.negotiatedCurrent_mA),
                    static_cast<unsigned long>(status.vbusVoltage_mV),
                    status.isPowerSourceDetected,
                    static_cast<unsigned long>(status.timeSinceSourceDetected_ms),
                    static_cast<unsigned long>(status.timeSinceLastPdDetected_ms));
  bsp::lampda_print("Charger use: %s, max %umA (PD full power %d, standard port %d)",
                    policy::can_use_power(status) ? "ALLOWED" : "blocked",
                    policy::max_input_current_mA(status),
                    policy::can_use_pd_full_power(status),
                    policy::is_standard_port(status));

  // raw CC levels: the board permanent Rd halves the CC voltage, a default Rp source can read as open
  int cc1 = 0;
  int cc2 = 0;
  fusb302_get_last_cc(&cc1, &cc2);
  bsp::lampda_print("CC levels (last read): CC1 %s, CC2 %s", cc_level_str(cc1), cc_level_str(cc2));

  const auto& c = info.counters;
  bsp::lampda_print(
          "Events: attach %lu, caps rx %lu, contracts %lu, soft reset tx %lu (fail %lu), hard reset tx %lu (fail %lu), "
          "hard reset rx %lu",
          static_cast<unsigned long>(c.attaches),
          static_cast<unsigned long>(c.src_caps_received),
          static_cast<unsigned long>(c.contracts),
          static_cast<unsigned long>(c.soft_resets_sent),
          static_cast<unsigned long>(c.soft_resets_failed),
          static_cast<unsigned long>(c.hard_resets_sent),
          static_cast<unsigned long>(c.hard_resets_tx_failed),
          static_cast<unsigned long>(c.hard_resets_received));
  bsp::lampda_print("Interrupt: line %s, stuck line kicks %lu, alert reruns %lu",
                    is_ic_interrupt_pending() ? "LOW (pending)" : "high",
                    static_cast<unsigned long>(interruptStuckKicks_s),
                    static_cast<unsigned long>(interruptAlertReruns_s));
}

void show_pd_status()
{
  data.serial_show();
  show_pd_contract();
}

/**
 *
 *      HEADER FUNCTIONS BELOW
 *
 */

void interrupt_handle()
{
  // this thread only runs when signal is sent
  bsp::threads::wait_notification(0);

#ifdef USE_PD_ALGO_LOOP
  // only waken up on thread update
  tcpc_alert();

  // new interrupts were raised while reading the registers: the line stayed low, and there will be no new edge
  static constexpr uint8_t maxAlertRuns = 8;
  for (uint8_t i = 0; i < maxAlertRuns and is_ic_interrupt_pending(); ++i)
  {
    interruptAlertReruns_s++;
    tcpc_alert();
  }
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
  hal::delay_ms(10);
#endif

  // partner asked us to stop to pull current
  if (should_stop_vbus_charge())
  {
    bsp::powergates::disable_gates();
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
      bsp::lampda_print("prepare fast role swap");
      bsp::powergates::disable_gates();

      // force otg on, and prep vbus gate, all in this loop iteration (skip all safety steps !!!)
      bsp::charger::set_OTG_targets(5000, 1000);
      bsp::charger::enable_OTG();

      hal::gpio::DigitalPin dischargeVbus(hal::gpio::DigitalPin::GPIO::Output_DischargeVbus);
      hal::gpio::DigitalPin fastRoleSwap(hal::gpio::DigitalPin::GPIO::Output_VbusFastRoleSwap);
      dischargeVbus.set_high(true);
      fastRoleSwap.set_high(true);

      // wait until OTG drops to acceptable level
      uint64_t timeout = get_time().val + 500 * MSEC_US;
      while (get_time().val < timeout and bsp::charger::get_measurments().vbus_mV > 5500)
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
      hal::gpio::DigitalPin(hal::gpio::DigitalPin::GPIO::Output_VbusDirection).set_high(true);
      dischargeVbus.set_high(false);
      fastRoleSwap.set_high(false);
     bsp::powergates::enable_vbus_gate_DIRECT();
    }
    // after the activation turn, disable the flag
    else
      isPowerSourceDetected_s = false;

    return;
  }
  else if (isFastRoleSwap)
  {
    bsp::charger::disable_OTG();

    // enable gate direction
    hal::gpio::DigitalPin(hal::gpio::DigitalPin::GPIO::Output_VbusDirection).set_high(false);
   bsp::powergates::disable_gates();
  }
  isFastRoleSwap = false;
#endif
}

bool setup()
{
  // 0 is success
  if (hal::i2c::i2c_check_existence(devicePort, fusb302_I2C_SLAVE_ADDR) != 0)
  {
    return false;
  }

  pd_init();
  hal::delay_ms(5);
  pd_startup();

  hal::gpio::DigitalPin chargerPin(hal::gpio::DigitalPin::GPIO::Signal_PowerDelivery);
  chargerPin.attach_callback(ic_interrupt,
                             hal::gpio::DigitalPin::Interrupt::kFallingEdge); // normal high, so focus on falling edge

  isSetup = true;
  return true;
}

void start_threads()
{
  if (!isSetup)
    return;

  static constexpr uint16_t taskSchedulerThreadBufferSize = 255;
  static bsp::threads::TaskBuffer_t taskSchedulerThreadBuffer[taskSchedulerThreadBufferSize];

  static constexpr uint16_t pdInterruptThreadBufferSize = 255;
  static bsp::threads::TaskBuffer_t pdInterruptThreadBuffer[pdInterruptThreadBufferSize];

  static constexpr uint16_t powerDeliveryThreadBufferSize = 1024;
  static bsp::threads::TaskBuffer_t powerDeliveryThreadBuffer[powerDeliveryThreadBufferSize];

  // start task scheduler, in suspended state
  bsp::threads::start_thread(task_scheduler,
                             bsp::threads::taskScheduler_taskName,
                             2,
                             taskSchedulerThreadBufferSize,
                             taskSchedulerThreadBuffer);
  // start interrupt handle, in suspended state
  bsp::threads::start_thread(interrupt_handle,
                             bsp::threads::pdInterruptHandle_taskName,
                             2,
                             pdInterruptThreadBufferSize,
                             pdInterruptThreadBuffer);
  // start pd handle loop
  bsp::threads::start_thread(
          pd_run, bsp::threads::pd_taskName, 1, powerDeliveryThreadBufferSize, powerDeliveryThreadBuffer);

  // service the interrupts raised before the threads existed (the notifications were lost)
  ic_interrupt();
}

void loop()
{
  // safety net: an interrupt edge was missed, the line is stuck low.
  // Throttled: the interrupt thread is the normal path, this must not flood the i2c line
  static uint32_t lastInterruptKick_ms = 0;
  if (hal::time_ms() - lastInterruptKick_ms >= 5 and is_ic_interrupt_pending())
  {
    lastInterruptKick_ms = hal::time_ms();
    interruptStuckKicks_s++;
    ic_interrupt();
  }

  data.update();

  // update battery level
  set_battery_level(static_cast<uint8_t>(component::battery::get_battery_level() / 100));

  // ignore source activity if we are otg (prevent spurious reset).
  // The PD state machine only flags the OTG activation when a sink is attached, but the charger can output on VBUS
  // without one (manual battery mode, or right after a detachment): our own VBUS is not a power source
  if (is_switching_to_otg() or bsp::charger::is_in_OTG())
  {
    return;
  }

  static policy::VbusLossTracker vbusLoss;

  // source detected
  const uint32_t time = hal::time_ms();
  if (is_vbus_powered())
  {
    if (not isPowerSourceDetected_s)
    {
      powerSourceDetectedTime_s = time;
      isPowerSourceDetected_s = true;
    }
    vbusLoss.on_vbus_valid(time);
  }
  else
  {
    // vbus can become briefly invalid when using the ICO algorithm, and the source drops it on a hard reset (up to ~2s,
    // this is not an unplug). Always updated (not short circuited), so that a hard reset during the loss is not missed.
    // The hard reset grace is bounded, never trust the PD state alone: a stalled state machine would keep the source
    // detected forever (issue #642)
    const bool isUnplugged = vbusLoss.is_unplugged(time, is_sink_hard_resetting() != 0);

    // deglitch time
    if (isPowerSourceDetected_s and
        // plugged in power can flicker
        time - powerSourceDetectedTime_s > 100 and isUnplugged)
    {
      isPowerSourceDetected_s = false;
      reset_cache();
    }
  }

  // no deglitch, this needs to be reactive
  canUseSourcePower_s = policy::can_use_power(get_source_status());
}

void shutdown() { pd_turn_off(); }

uint16_t get_max_input_current() { return policy::max_input_current_mA(get_source_status()); }

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

std::array<PDOTypes, 5> get_available_pd()
{
  std::array<PDOTypes, 5> pdos;
  uint8_t i = 0;
  for (; i < min<uint8_t>(5, get_pd_source_cnt()); ++i)
  {
    PDOTypes t;
    pd_extract_pdo_power(get_pd_source(i), &t.maxCurrent_mA, &t.voltage_mv);
    pdos[i] = t;
  }
  for (; i < 5; i++)
  {
    pdos[i] = PDOTypes();
  }
  return pdos;
}

} // namespace powerDelivery
} // namespace bsp
} // namespace lampda

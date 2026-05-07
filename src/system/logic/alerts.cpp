#include "alerts.h"

#include "src/system/platform/time.h"
#include "src/system/platform/bluetooth.h"
#include "src/system/platform/registers.h"
#include "src/system/platform/print.h"

#include "src/system/physical/indicator.h"
#include "src/system/physical/battery.h"

#include "src/system/logic/brightness_handle.h"
#include "src/system/logic/power_handler.h"
#include "src/system/logic/statistics_handler.h"

#include "src/system/utils/utils.h"
#include "src/system/utils/constants.h"
#include "src/system/utils/time_utils.h"

#include "src/system/power/charger.h"
#include "src/system/power/balancer.h"

namespace lampda {
namespace logic {
namespace alerts {

/// Instanciation of the AlertManager
AlertManager_t manager;

/// Set to true if an alert requested an emmergency shutdown
bool _request_shutdown = false;

/// Stay at zero in normal operation.
/// Set to the real startup time to ignore the battery alerts while the system starts
uint32_t _startupChargerTime = 0;

/// Return true if we are ready to display battery alerts
bool is_battery_alert_ready()
{
  const bool isReady = _startupChargerTime == 0 or (platform::time_ms() - _startupChargerTime) > 5000;
  if (isReady)
    _startupChargerTime = 0;
  return isReady;
}

/// Convert alerts enums to text
inline const char* AlertsToText(const Type type)
{
  switch (type)
  {
    case Type::MAIN_LOOP_FREEZE:
      return "MAIN_LOOP_FREEZE";
    case Type::BATTERY_READINGS_INCOHERENT:
      return "BATTERY_READINGS_INCOHERENT";
    case Type::BATTERY_CRITICAL:
      return "BATTERY_CRITICAL";
    case Type::BATTERY_LOW:
      return "BATTERY_LOW";
    case Type::LONG_LOOP_UPDATE:
      return "LONG_LOOP_UPDATE";
    case Type::TEMP_TOO_HIGH:
      return "TEMP_TOO_HIGH";
    case Type::TEMP_CRITICAL:
      return "TEMP_CRITICAL";
    case Type::BLUETOOTH_ADVERT:
      return "BLUETOOTH_ADVERT";
    case Type::HARDWARE_ALERT:
      return "HARDWARE_ALERT";
    case Type::FAVORITE_SET:
      return "FAVORITE_SET";
    case Type::OTG_FAILED:
      return "OTG_FAILED";
    case Type::SYSTEM_OFF_FAILED:
      return "SYSTEM_OFF_FAILED";
    case Type::SYSTEM_IN_ERROR_STATE:
      return "SYSTEM_IN_ERROR_STATE";
    case Type::SYSTEM_IN_LOCKOUT:
      return "SYSTEM_IN_LOCKOUT";
    case Type::SUNSET_TIMER_ENABLED:
      return "SUNSET_TIMER_ENABLED";
    case Type::SYSTEM_SLEEP_SKIPPED:
      return "SYSTEM_SLEEP_SKIPPED";
    case Type::USB_PORT_SHORT:
      return "USB_PORT_SHORT";
    case Type::BATTERY_MISSING:
      return "BATTERY_MISSING";
    case Type::CHARGER_ERROR:
      return "CHARGER_ERROR";
    default:
      return "UNSUPPORTED TYPE";
  }
}

namespace __internal {

/// only sample battery level every N steps
uint16_t get_battery_level()
{
  static uint16_t lastPercent = 0;

  EVERY_N_MILLIS(1000.0)
  {
    const uint16_t newPercent = physical::battery::get_battery_minimum_cell_level();
    if ((lastPercent / 100) != (newPercent / 100))
    {
      platform::bluetooth::write_battery_level(static_cast<uint8_t>(newPercent / 100));
    }
    lastPercent = newPercent;
  }
  return lastPercent;
}

} // namespace __internal

/**
 * \brief Base class for all alerts.
 * Must be overloaded to define an alert.
 */
struct AlertBase
{
  /// virtual destructor for a pure virtual class
  virtual ~AlertBase() = default;

  /// Return the timeout after which an alert will make the system auto-shutdown
  virtual uint32_t alert_shutdown_timeout() const
  {
    // max timeout, will never be exeeded
    return UINT32_MAX;
  }

  /// function executed when this alert is raised
  virtual void execute() const {};

  /// return true if this alert type should be raised
  virtual bool should_be_raised() const { return false; }

  /// return true if this alert type should be cleared
  virtual bool should_be_cleared() const { return false; }

  /// display this alert on the indicator
  virtual bool show() const { return physical::indicator::blink(300, 300, utils::ColorSpace::WHITE); }

  /// return the Defined type of this alert
  virtual Type get_type() const = 0;

  /// Return true if the system should shutdown from this alert
  virtual bool should_shutdown_system(const uint32_t time) final
  {
    return (time - raisedTime) > alert_shutdown_timeout();
  }

  /**
   * \brief Handler for the raised state of the alert.
   * If the alert is not raised, raise the alert.
   * \return True if the alert was raised by this function call
   */
  virtual bool handle_raised_state(const uint32_t time)
  {
    if (not _isRaisedHandled)
    {
      raisedTime = time;
      loweredTime = 0;
      _isRaisedHandled = true;
      _isLoweredHandled = false;

      return true;
    }

    return false;
  }

  /// Update the raised time of this alert.
  /// An alert continuously firing as to be updated
  void update_raise_time(const uint32_t time) { raisedTime = time; }

  /**
   * \brief Handler for the lowered state of the alert.
   * If the alert is raised, lower the alert.
   * \return True if the alert was lowered by this function call
   */
  virtual bool handle_lowered_state(const uint32_t time)
  {
    if (not _isLoweredHandled)
    {
      raisedTime = 0;
      loweredTime = time;
      _isLoweredHandled = true;
      _isRaisedHandled = false;
      return true;
    }
    return false;
  }

  /// override to prevent lamp output power if this alert if raised. Default to False
  virtual bool should_prevent_lamp_output() const { return false; }
  /// override to prevent battery charge if this alert if raised. Default to False
  virtual bool should_prevent_battery_charge() const { return false; }
  /// override to prevent USB port usage if this alert if raised. Default to False
  virtual bool should_prevent_usb_port_use() const { return false; }

  // private:
  //
  /// Alert raise signal has been handled
  bool _isRaisedHandled = false;
  /// Alert lower signal has been handled
  bool _isLoweredHandled = false;
  /// Store the latest time at which this alert raise() was called
  uint32_t raisedTime = 0;
  /// Store the latest time at which this alert lower() was called
  uint32_t loweredTime = 0;
};

/**
 * \brief This alert is raised when the main loop becomes unresponsive
 */
struct Alert_MainLoopFreeze : public AlertBase
{
  Type get_type() const override { return Type::MAIN_LOOP_FREEZE; }
};

/**
 * \brief This alert is raised when the battery readings do not fit the battery physical model
 */
struct Alert_BatteryReadingIncoherent : public AlertBase
{
  bool show() const override
  {
    return physical::indicator::blink(100, 100, {utils::ColorSpace::GREEN, utils::ColorSpace::RED});
  }

  Type get_type() const override { return Type::BATTERY_READINGS_INCOHERENT; }

  bool should_be_cleared() const override
  {
    // cleared after a delay
    return raisedTime > 0 and (platform::time_ms() - raisedTime) > 2000;
  }

  bool should_prevent_lamp_output() const override { return true; }
  bool should_prevent_battery_charge() const override { return true; }
  bool should_prevent_usb_port_use() const override { return true; }
};

/**
 * \brief This alert is raised when the battery level falls below a critical threshold. Falling further will dammage the
 * battery
 */
struct Alert_BatteryCritical : public AlertBase
{
  bool should_be_raised() const override
  {
    if (not is_battery_alert_ready())
      return false;
    if (logic::power::is_in_error_state())
      return false;
    const auto& chargerState = ::lampda::power::charger::get_state();
    if (not chargerState.areMeasuresOk)
      return false;

// TODO issue #132 remove when the mock components will be running
#ifndef LMBD_SIMULATION
    if (not ::lampda::power::balancer::get_status().is_valid())
      return false;
#endif

    if (manager.is_raised(Type::BATTERY_MISSING) or manager.is_raised(Type::BATTERY_READINGS_INCOHERENT))
      return false;

    return not chargerState.is_effectivly_charging() and __internal::get_battery_level() < batteryCritical;
  }

  bool should_be_cleared() const override
  {
    // incoherent means no other battery alerts
    if (manager.is_raised(Type::BATTERY_READINGS_INCOHERENT))
      return true;
    // battery low can only be cleared on charging operations
    const auto& chargerState = ::lampda::power::charger::get_state();
    return chargerState.is_effectivly_charging();
  }

  /// If this alert is raised, system should turn off fast
  uint32_t alert_shutdown_timeout() const override
  {
    // shutdown after 2 seconds
    return 2000;
  }

  bool show() const override
  {
    // fast blink red
    return physical::indicator::blink(100, 100, utils::ColorSpace::RED);
  }

  Type get_type() const override { return Type::BATTERY_CRITICAL; }
};

/**
 * \brief This alert is raised when the battery level gets low.
 */
struct Alert_BatteryLow : public AlertBase
{
  bool should_be_raised() const override
  {
    if (not is_battery_alert_ready())
      return false;
    if (logic::power::is_in_error_state())
      return false;

// TODO issue #132 remove when the mock components will be running
#ifndef LMBD_SIMULATION
    if (not ::lampda::power::balancer::get_status().is_valid())
      return false;
#endif

    const auto& chargerState = ::lampda::power::charger::get_state();
    if (not chargerState.areMeasuresOk)
      return false;
    if (manager.is_raised(Type::BATTERY_MISSING) or manager.is_raised(Type::BATTERY_READINGS_INCOHERENT))
      return false;

    const auto& batteryLevel = __internal::get_battery_level();
    const bool isBatteryLow = not chargerState.is_effectivly_charging() and batteryLevel < batteryLow;
    // battery low will be raise, notify bluetooth
    if (isBatteryLow)
      platform::bluetooth::notify_battery_level(static_cast<uint8_t>(batteryLevel / 100));
    return isBatteryLow;
  }

  bool should_be_cleared() const override
  {
    // incoherent means no other battery alerts
    if (manager.is_raised(Type::BATTERY_READINGS_INCOHERENT))
      return true;

    // battery low can only be cleared on charging operations
    const auto& chargerState = ::lampda::power::charger::get_state();
    return chargerState.is_effectivly_charging();
  }

  /// Execution will lower the brigthness output
  void execute() const override
  {
    // limit brightness to quarter of the max value
    constexpr brightness_t clampedBrightness =
            static_cast<brightness_t>(0.25 * ::lampda::brightness::absoluteMaximumBrightness);

    // save some battery
    platform::bluetooth::stop_bluetooth_advertising();

    logic::brightness::set_max_brightness(clampedBrightness);
    logic::brightness::update_brightness(logic::brightness::get_saved_brightness());
    logic::brightness::update_saved_brightness();
  }

  bool show() const override
  {
    // fast blink red
    return physical::indicator::blink(300, 300, utils::ColorSpace::RED);
  }

  Type get_type() const override { return Type::BATTERY_LOW; }
};

/**
 * \brief This alert is raised when no connected battery is detected.
 */
struct Alert_BatteryMissing : public AlertBase
{
  bool show() const override
  {
    //
    return physical::indicator::blink(100, 100, {utils::ColorSpace::GREEN, utils::ColorSpace::RED});
  }

  Type get_type() const override { return Type::BATTERY_MISSING; }

  /// Prevent lamp output
  bool should_prevent_lamp_output() const override { return true; }
  /// Prevent battery charge
  bool should_prevent_battery_charge() const override { return true; }
  /// Prevent USB port use
  bool should_prevent_usb_port_use() const override { return true; }
};

/**
 * \brief This alert is raised when the main loop becomes slower than the defined behavior.
 * This is not a critical alert, but it sually indicates that the user mode in use is too slow.
 */
struct Alert_LongLoopUpdate : public AlertBase
{
  bool show() const override { return physical::indicator::blink(400, 400, utils::ColorSpace::FUSHIA); }

  Type get_type() const override { return Type::LONG_LOOP_UPDATE; }
};

/**
 * \brief This alert is raised when the internal temperature gets high.
 * If it keeps rising, the battery may get dammaged.
 */
struct Alert_TempTooHigh : public AlertBase
{
  bool should_be_raised() const override
  {
    // raised above a threshold
    return platform::registers::read_CPU_temperature_degreesC() >= maxSystemTemp_c;
  }

  bool should_be_cleared() const override
  {
    // cleared below a threshold
    return platform::registers::read_CPU_temperature_degreesC() <= maxSystemTemp_c * 0.75;
  }

  /// Execution will lower the max brightness of the output
  void execute() const override
  {
    // limit brightness to half the max value
    constexpr brightness_t clampedBrightness =
            static_cast<brightness_t>(0.5 * ::lampda::brightness::absoluteMaximumBrightness);

    logic::brightness::set_max_brightness(clampedBrightness);
    logic::brightness::update_brightness(logic::brightness::get_saved_brightness());
    logic::brightness::update_saved_brightness();
  }

  bool show() const override { return physical::indicator::blink(300, 300, utils::ColorSpace::DARK_ORANGE); }

  Type get_type() const override { return Type::TEMP_TOO_HIGH; }
};

/**
 * \brief This alert is raised when the temperature gets too high.
 * If it keeps climbing, the components will get dammaged.
 */
struct Alert_TempCritical : public AlertBase
{
  bool should_be_raised() const override
  {
    // raised above a threshold
    return platform::registers::read_CPU_temperature_degreesC() >= criticalSystemTemp_c;
  }

  /// Shutdown fast
  uint32_t alert_shutdown_timeout() const override
  {
    // shutdown, critical temp is the absolute limit
    return 5000;
  }

  bool show() const override { return physical::indicator::blink(100, 100, utils::ColorSpace::DARK_ORANGE); }

  Type get_type() const override { return Type::TEMP_CRITICAL; }

  /// Prevent lamp output
  bool should_prevent_lamp_output() const override { return true; }
  /// prevent battery charging
  bool should_prevent_battery_charge() const override { return true; }
  /// prevent USB port use
  bool should_prevent_usb_port_use() const override { return true; }
};

/**
 * \brief This alert is raised when the Bluetooth advertising is turned on.
 * It is just an informative alert.
 */
struct Alert_BluetoothAdvertisement : public AlertBase
{
  bool show() const override { return physical::indicator::breeze(1000, 500, utils::ColorSpace::BLUE); }

  Type get_type() const override { return Type::BLUETOOTH_ADVERT; }
};

/**
 * \brief This alert is raised when a problem is detected in the hardware.
 * Usually, it means that an electrical component may have become unresponsive.
 */
struct Alert_HardwareAlert : public AlertBase
{
  bool show() const override
  {
    return physical::indicator::blink(100, 100, {utils::ColorSpace::PURPLE, utils::ColorSpace::TEAL});
  }

  Type get_type() const override { return Type::HARDWARE_ALERT; }

  /// prevent usage of the output
  bool should_prevent_lamp_output() const override { return true; }
  /// prevent battery charge
  bool should_prevent_battery_charge() const override { return true; }
  /// prevent USB use
  bool should_prevent_usb_port_use() const override { return true; }
};

/**
 * \brief This alert is raised when the battery charger signals an error.
 * It is not well handled at the moment, so it does not block anything.
 */
struct Alert_ChargerError : public AlertBase
{
  bool show() const override
  {
    return physical::indicator::blink(100, 100, {utils::ColorSpace::WHITE, utils::ColorSpace::BLACK});
  }

  Type get_type() const override { return Type::CHARGER_ERROR; }
};

/**
 * \brief This alert is raised when new favorite mode is added.
 */
struct Alert_FavoriteSet : public AlertBase
{
  bool show() const override { return physical::indicator::blink(100, 100, utils::ColorSpace::TEAL); }

  Type get_type() const override { return Type::FAVORITE_SET; }

  /// It is automatically cleared after a delay
  bool should_be_cleared() const override
  {
    // cleared after a delay
    return raisedTime > 0 and (platform::time_ms() - raisedTime) > 1000;
  }
};

/**
 * \brief This alert is raised when the output power fails to start.
 * It can happen with logic bugs, or if the output rail is shorted.
 */
struct Alert_OtgFailed : public AlertBase
{
  bool show() const override
  {
    return physical::indicator::blink(300, 200, {utils::ColorSpace::BLUE, utils::ColorSpace::YELLOW});
  }

  Type get_type() const override { return Type::OTG_FAILED; }

  /// Block power output
  bool should_prevent_lamp_output() const override { return true; }
};

/**
 * \brief This alert is raised when the system fails to go to sleep.
 * There is no way to recover from this, as we depend on the low level handlers for this functionality.
 */
struct Alert_SystemShutdownFailed : public AlertBase
{
  bool show() const override
  {
    return physical::indicator::blink(100, 100, {utils::ColorSpace::PURPLE, utils::ColorSpace::WHITE});
  }

  Type get_type() const override { return Type::SYSTEM_OFF_FAILED; }

  /// Cleared after a delay
  bool should_be_cleared() const override
  {
    if (raisedTime > 0 and (platform::time_ms() - raisedTime) > 2000)
    {
      // is this fails, the system is just too broken to be repaired
      platform::registers::enter_serial_dfu();
      return true;
    }
    return false;
  }

  /// Prevent output usage
  bool should_prevent_lamp_output() const override { return true; }
  /// Prevent battery charge
  bool should_prevent_battery_charge() const override { return true; }
  /// Prevent USB use
  bool should_prevent_usb_port_use() const override { return true; }
};

/**
 * \brief This alert is raised when the system falls into an unrecoverable state.
 * It can be raised by logic error, or detected bugs at program start
 */
struct Alert_SystemInErrorState : public AlertBase
{
  bool show() const override
  {
    return physical::indicator::blink(100, 100, {utils::ColorSpace::PINK, utils::ColorSpace::ORANGE});
  }

  Type get_type() const override { return Type::SYSTEM_IN_ERROR_STATE; }

  /// Prevent output usage
  bool should_prevent_lamp_output() const override { return true; }
  /// Prevent battery usage
  bool should_prevent_battery_charge() const override { return true; }
  /// prevent USB usage
  bool should_prevent_usb_port_use() const override { return true; }
};

/**
 * \brief This alert is raised when the user tries to power the system as it is in lockout mode.
 */
struct Alert_SystemInLockout : public AlertBase
{
  bool show() const override
  {
    return physical::indicator::blink(
            200,
            50,
            {utils::ColorSpace::BLACK, utils::ColorSpace::BLUE, utils::ColorSpace::WHITE, utils::ColorSpace::RED});
  }

  Type get_type() const override { return Type::SYSTEM_IN_LOCKOUT; }
};

/**
 * \brief This alert is raised when the sunset timer is active.
 * It means that the system will turn off on it's own after a delay.
 */
struct Alert_SunsetTimerSet : public AlertBase
{
  bool show() const override
  {
    // red to green
    const auto& batteryLevel = __internal::get_battery_level();
    const auto buttonColor = utils::ColorSpace::RGB(utils::get_gradient(
            utils::ColorSpace::RED.get_rgb().color, utils::ColorSpace::GREEN.get_rgb().color, batteryLevel / 10000.0f));

    return physical::indicator::breeze(5000, 5000, buttonColor);
  }

  Type get_type() const override { return Type::SUNSET_TIMER_ENABLED; }
};

/**
 * \brief This alert is raised when the system starts without sleeping cleanly first.
 * It appears after updates, crashes, power failures, ...
 */
struct Alert_SkippedCleanSleep : public AlertBase
{
  bool show() const override { return physical::indicator::blink(250, 250, utils::ColorSpace::PINK); }

  Type get_type() const override { return Type::SYSTEM_SLEEP_SKIPPED; }

  /// Auto cleared after a delay
  bool should_be_cleared() const override
  {
    // cleared after a delay
    return (raisedTime > 0 and (platform::time_ms() - raisedTime) > 3000);
  }
};

/**
 * \brief This alert is raised when a short circuit is detected in the USB lines.
 * The port should be cleaned before using it again.
 */
struct Alert_UsbPortShort : public AlertBase
{
  bool show() const override { return physical::indicator::blink(100, 100, utils::ColorSpace::BLUE); }

  Type get_type() const override { return Type::USB_PORT_SHORT; }

  /// auto cleared after a delay
  bool should_be_cleared() const override { return (raisedTime > 0 and (platform::time_ms() - raisedTime) > 5000); }

  /// Prevent charge
  bool should_prevent_battery_charge() const override { return true; }
  /// Prevent USB use
  bool should_prevent_usb_port_use() const override { return true; }
};

/// Alerts must be sorted by importance, only the first activated one will be shown
AlertBase* allAlerts[] = {
        new Alert_SystemShutdownFailed,
        new Alert_BatteryMissing, // if battery is missing, the system will also have the hardware alert
        new Alert_HardwareAlert,
        new Alert_TempCritical,
        new Alert_UsbPortShort,
        //
        new Alert_SkippedCleanSleep,
        // battery and temp related
        new Alert_BatteryCritical,
        new Alert_TempTooHigh,
        new Alert_BatteryReadingIncoherent,
        new Alert_BatteryLow,
        //
        new Alert_OtgFailed,
        new Alert_SystemInErrorState,
        new Alert_SystemInLockout,
        // user side low priority alerts
        new Alert_LongLoopUpdate,
        new Alert_BluetoothAdvertisement,
        new Alert_FavoriteSet,
        new Alert_SunsetTimerSet,

        new Alert_ChargerError};

void update_alerts()
{
  const uint32_t currTime = platform::time_ms();
  for (auto alert: allAlerts)
  {
    if (manager.is_raised(alert->get_type()))
    {
      if (alert->should_be_cleared())
      {
        manager.clear(alert->get_type());
      }
      else
      {
        // call this before any other processes, sets the delays
        if (alert->handle_raised_state(currTime))
        {
          // execute this alert action
          alert->execute();
        }

        if (alert->should_shutdown_system(currTime))
        {
          // notify the system shutdown request
          _request_shutdown = true;
        }
      }
    }
    else
    {
      // alert is cleared, check if it needs to be raised
      if (alert->should_be_raised())
      {
        manager.raise(alert->get_type());
        platform::lampda_print("Raised alert %s", AlertsToText(alert->get_type()));
      }
      else
      {
        alert->handle_lowered_state(currTime);
      }
    }
  }
}

void signal_wake_up_from_charger() { _startupChargerTime = platform::time_ms(); }

void handle_all(const bool shouldIgnoreAlerts)
{
  // update all alerts
  update_alerts();

  const bool isNoAlertsRaised = manager.is_clear();

  // no alerts: reset the max brightness
  if (isNoAlertsRaised)
  {
    // This should be called on no alerts only, not ignore alerts
    logic::brightness::set_max_brightness(::lampda::brightness::absoluteMaximumBrightness);
  }

  //
  if (shouldIgnoreAlerts or isNoAlertsRaised)
  {
    // do nothing to display anything
    if (not logic::indicator::should_indicator_be_visible())
    {
      physical::indicator::set_color(utils::ColorSpace::BLACK);
      return;
    }

    // red to green
    const auto& batteryLevel = __internal::get_battery_level();
    const auto buttonColor = utils::ColorSpace::RGB(utils::get_gradient(
            utils::ColorSpace::RED.get_rgb().color, utils::ColorSpace::GREEN.get_rgb().color, batteryLevel / 10000.0f));

    // display battery level
    const auto& chargerStatus = ::lampda::power::charger::get_state();
    if (!logic::power::is_in_output_mode() and chargerStatus.isInOtg)
    {
      physical::indicator::breeze(500, 500, buttonColor);
    }
    else if (chargerStatus.is_charging())
    {
      // power detected with no charge or slow charging raises a special animation
      if (chargerStatus.status == ::lampda::power::charger::Charger_t::ChargerStatus_t::POWER_DETECTED or
          chargerStatus.status == ::lampda::power::charger::Charger_t::ChargerStatus_t::SLOW_CHARGING)
      {
        // fast blinking
        physical::indicator::blink(500, 500, buttonColor);
      }
      // standard charge mode
      else
      {
        physical::indicator::breeze(2000, 1000, buttonColor);
      }
    }
    // output mode, or end of charge : standard display
    else if (logic::power::is_in_output_mode() or chargerStatus.is_charge_finished())
    {
      // normal output mode
      // chargerStatus.isInOtg should be true
      physical::indicator::set_color(buttonColor);
    }
    else
    {
      // we can handup here when starting/shutting down the system
      // no charger operation, no output mode
      physical::indicator::blink(1000, 1000, buttonColor);
    }

    // skip the other alerts
    return;
  }

  bool isFirstAlertShown = false;
  for (auto alert: allAlerts)
  {
    if (manager.is_raised(alert->get_type()))
    {
      // display only the first alert
      isFirstAlertShown = true;
      alert->show();
      break;
    }
  }

  // unhandled case (white blink)
  if (not isFirstAlertShown)
  {
    physical::indicator::blink(300, 300, utils::ColorSpace::WHITE);
  }
}

void show_all()
{
  if (manager.is_clear())
  {
    platform::lampda_print("No alerts raised");
  }
  else
  {
    platform::lampda_print("Raised alerts:");
    for (auto alert: allAlerts)
    {
      if (manager.is_raised(alert->get_type()))
      {
        platform::lampda_print("- %s", AlertsToText(alert->get_type()));
      }
    }
  }
}

bool is_request_shutdown() { return _request_shutdown; }

void AlertManager_t::raise(const Type type)
{
  if (is_raised(type))
  {
    // update raise time
    for (auto& alert: allAlerts)
    {
      if (type == alert->get_type())
      {
        alert->update_raise_time(platform::time_ms());
        break;
      }
    }
    return;
  }
  else
  {
    statistics::signal_alert_raised(static_cast<uint32_t>(type));
  }

  platform::lampda_print("ALERT raised: %s", AlertsToText(type));
  _current |= static_cast<uint32_t>(type);
}

void AlertManager_t::clear(const Type type)
{
  if (not is_raised(type))
    return;
  platform::lampda_print("ALERT cleared: %s", AlertsToText(type));
  _current ^= static_cast<uint32_t>(type);
}

uint32_t AlertManager_t::get_time_since_raised(const Type type) const
{
  for (auto alert: allAlerts)
  {
    if (alert->get_type() == type)
    {
      // safety if the alert was raised in between, or not yet handled
      if (not is_raised(type) or not alert->_isRaisedHandled)
        return 0;
      // compute raised time
      return platform::time_ms() - alert->raisedTime;
    }
  }
  return 0;
}

bool AlertManager_t::can_use_output_power() const
{
  if (is_clear())
    return true;

  for (auto alert: allAlerts)
  {
    if (alert->_isRaisedHandled && alert->should_prevent_lamp_output())
    {
      return false;
    }
  }
  return true;
}

bool AlertManager_t::can_charge_battery() const
{
  if (is_clear())
    return true;

  for (auto alert: allAlerts)
  {
    if (alert->_isRaisedHandled && alert->should_prevent_battery_charge())
    {
      return false;
    }
  }
  return true;
}

bool AlertManager_t::can_use_usb_port() const
{
  if (is_clear())
    return true;

  for (auto alert: allAlerts)
  {
    if (alert->_isRaisedHandled && alert->should_prevent_usb_port_use())
    {
      return false;
    }
  }
  return true;
}

} // namespace alerts
} // namespace logic
} // namespace lampda

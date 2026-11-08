#include "alerts.h"

#include "src/system/hal/time.h"
#include "src/system/hal/mutex.h"
#include "src/system/hal/registers.h"

#include "src/system/bsp/balancer.h"
#include "src/system/bsp/ble.h"
#include "src/system/bsp/indicator.h"
#include "src/system/bsp/text_out.h"

#include "src/system/component/battery.h"
#include "src/system/component/charger.h"

#include "src/system/logic/brightness_handle.h"
#include "src/system/logic/behavior.h"
#include "src/system/logic/power_handler.h"
#include "src/system/logic/statistics_handler.h"
#include "src/system/logic/sun_timers.h"

#include "src/system/utils/utils.h"
#include "src/system/utils/constants.h"
#include "src/system/utils/time_utils.h"

namespace lampda {
namespace logic {
namespace alerts {

/// Instanciation of the AlertManager
AlertManager_t manager;

/// Guards the raise times of all alerts (AlertBase::raisedTime): refreshed by raise() from any thread (power thread,
/// BLE, ...), while the main loop takes its timeout decisions on them.
/// The elapsed times are computed under the lock, so a refresh can never be seen as "in the future" of the decision.
/// Not recursive.
static hal::mutex::hal_mutex_t _raiseTimeMutex;
/// Initialized at program start: alerts can be raised before the main setup
[[maybe_unused]] static const bool _isRaiseTimeMutexInit = (hal::mutex::hal_mutex_init(&_raiseTimeMutex), true);

/// Hold _raiseTimeMutex for the scope
struct RaiseTimeLock
{
  RaiseTimeLock() { hal::mutex::hal_mutex_lock(&_raiseTimeMutex); }
  ~RaiseTimeLock() { hal::mutex::hal_mutex_unlock(&_raiseTimeMutex); }
  RaiseTimeLock(const RaiseTimeLock&) = delete;
  RaiseTimeLock& operator=(const RaiseTimeLock&) = delete;
};

/// Set to true if an alert requested an emmergency shutdown
bool _request_shutdown = false;

/// Stay at zero in normal operation.
/// Set to the real startup time to ignore the battery alerts while the system starts
uint32_t _startupChargerTime = 0;

/// Return true if we are ready to display battery alerts
bool is_battery_alert_ready()
{
  const bool isReady = _startupChargerTime == 0 or (hal::time_ms() - _startupChargerTime) > 5000;
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
    case Type::OTG_OVER_LIMITS:
      return "OTG_OVER_LIMITS";
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
    const uint16_t newPercent = component::battery::get_battery_minimum_cell_level();
    if ((lastPercent / 100) != (newPercent / 100))
    {
      bsp::ble::write_battery_level(static_cast<uint8_t>(newPercent / 100));
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

  /// function executed if this alert is raised, every loop turns
  virtual void execute_loop() const {};

  /// function executed when this alert is raised
  virtual void on_raised() const {};
  /// function executed when this alert is cleared
  virtual void on_lowered() const {};

  /// return true if this alert type should be raised
  virtual bool should_be_raised() const { return false; }

  /// return true if this alert type should be cleared
  virtual bool should_be_cleared() const { return false; }

  /// display this alert on the indicator
  virtual bool show() const { return bsp::indicator::blink(300, 300, utils::ColorSpace::WHITE); }

  /// return the Defined type of this alert
  virtual Type get_type() const = 0;

  /// Return true if the system should shutdown from this alert
  virtual bool should_shutdown_system() final
  {
    const auto shutdownTimeout = alert_shutdown_timeout();
    if (shutdownTimeout == UINT32_MAX)
      return false;
    return time_since_raised() > shutdownTimeout;
  }

  /// Time elapsed since the latest raise (or refresh) of this alert, 0 if the raise time is not set.
  /// Synchronized with the raise time refreshes. Can be called from any thread.
  uint32_t time_since_raised() const
  {
    RaiseTimeLock lock;
    // the validity is tracked apart: a raise at the clock value 0 is a valid raise time
    if (not isRaisedTimeSet)
      return 0;
    // time read under the lock: always after the latest refresh
    return hal::time_ms() - raisedTime;
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
      {
        RaiseTimeLock lock;
        // keep the latest of the two: a refresh since \p time was captured must not be overwritten by an older time
        // (elapsed time comparison, safe across a time_ms wrap)
        if (not isRaisedTimeSet or static_cast<int32_t>(time - raisedTime) > 0)
          raisedTime = time;
        isRaisedTimeSet = true;
      }
      loweredTime = 0;
      _isRaisedHandled = true;
      _isLoweredHandled = false;

      return true;
    }

    return false;
  }

  /// Update the raised time of this alert.
  /// An alert continuously firing as to be updated. Call it with _raiseTimeMutex held (see AlertManager_t::raise).
  void update_raise_time_locked()
  {
    // time read under the lock: never older than a raise time set before
    raisedTime = hal::time_ms();
    isRaisedTimeSet = true;
    // signal the refresh to a clear decision taken concurrently, see AlertManager_t::clear_unless_refreshed
    refreshCount.fetch_add(1);
  }

  /**
   * \brief Handler for the lowered state of the alert.
   * If the alert is raised, lower the alert.
   * \return True if the alert was lowered by this function call
   */
  virtual bool handle_lowered_state(const uint32_t time)
  {
    if (not _isLoweredHandled)
    {
      {
        RaiseTimeLock lock;
        raisedTime = 0;
        isRaisedTimeSet = false;
      }
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
  /// Store the latest time at which this alert raise() was called, 0 if not set.
  /// Refreshed by raise() from other threads, while the main loop reads it for the timeout decisions:
  /// only access it under _raiseTimeMutex (see time_since_raised())
  uint32_t raisedTime = 0;
  /// True if raisedTime holds a raise time (that can be 0). Only access it under _raiseTimeMutex
  bool isRaisedTimeSet = false;
  /// Incremented on every raise() refresh of the raised alert, under _raiseTimeMutex.
  /// See AlertManager_t::clear_unless_refreshed
  std::atomic<uint32_t> refreshCount {0};
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
    return bsp::indicator::blink(100, 100, {utils::ColorSpace::GREEN, utils::ColorSpace::RED});
  }

  Type get_type() const override { return Type::BATTERY_READINGS_INCOHERENT; }

  bool should_be_cleared() const override
  {
    // cleared after a delay
    return time_since_raised() > 2000;
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
    const auto& chargerState = ::lampda::component::charger::get_state();
    if (not chargerState.areMeasuresOk)
      return false;

    if (not ::lampda::bsp::balancer::get_status().is_valid())
      return false;

    if (manager.is_raised(Type::BATTERY_MISSING) or manager.is_raised(Type::BATTERY_READINGS_INCOHERENT))
      return false;

    return not chargerState.is_effectivly_charging() and
           __internal::get_battery_level() < physical_constants::batteryCritical;
  }

  bool should_be_cleared() const override
  {
    // incoherent means no other battery alerts
    if (manager.is_raised(Type::BATTERY_READINGS_INCOHERENT))
      return true;
    // battery low can only be cleared on charging operations
    const auto& chargerState = ::lampda::component::charger::get_state();
    // use the charging standard set to prevent the lamp to turn off while debugging
    return chargerState.is_effectivly_charging();
  }

  /// If this alert is raised, system should turn off fast
  uint32_t alert_shutdown_timeout() const override
  {
    // never shutdown in charge state
    if (logic::behavior::is_in_charge_state())
      return UINT32_MAX;
    // shutdown after 2 seconds
    return 2000;
  }

  bool show() const override
  {
    // fast blink red
    return bsp::indicator::blink(100, 100, utils::ColorSpace::RED);
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

    if (not ::lampda::bsp::balancer::get_status().is_valid())
      return false;

    const auto& chargerState = ::lampda::component::charger::get_state();
    if (not chargerState.areMeasuresOk)
      return false;
    if (manager.is_raised(Type::BATTERY_MISSING) or manager.is_raised(Type::BATTERY_READINGS_INCOHERENT))
      return false;

    const auto& batteryLevel = __internal::get_battery_level();
    const bool isBatteryLow =
            not chargerState.is_effectivly_charging() and batteryLevel < physical_constants::batteryLow;
    // battery low will be raise, notify bluetooth
    if (isBatteryLow)
      bsp::ble::notify_battery_level(static_cast<uint8_t>(batteryLevel / 100));
    return isBatteryLow;
  }

  bool should_be_cleared() const override
  {
    // incoherent means no other battery alerts
    if (manager.is_raised(Type::BATTERY_READINGS_INCOHERENT))
      return true;

    // battery low can only be cleared on charging operations
    const auto& chargerState = ::lampda::component::charger::get_state();
    return chargerState.is_effectivly_charging();
  }

  /// Execution will lower the brightness output
  void on_raised() const override
  {
    // limit brightness to quarter of the max value
    constexpr brightness_t clampedBrightness =
            static_cast<brightness_t>(0.25 * ::lampda::brightness::absoluteMaximumBrightness);

    logic::brightness::set_max_brightness(clampedBrightness);
    logic::brightness::update_brightness(logic::brightness::get_saved_brightness());
    logic::brightness::update_saved_brightness();

    // this alert may lead to a shutdown, start phasing out the sunset timer
    // If the user decides to set the timer up again, it wont be lowered again by this alert
    logic::sunset_timer.shortcut_to_brightness_phase();
  }

  bool show() const override
  {
    // fast blink red
    return bsp::indicator::blink(300, 300, utils::ColorSpace::RED);
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
    return bsp::indicator::blink(100, 100, {utils::ColorSpace::GREEN, utils::ColorSpace::RED});
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
  bool show() const override { return bsp::indicator::blink(400, 400, utils::ColorSpace::FUSHIA); }

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
    return hal::registers::read_CPU_temperature_degreesC() >= physical_constants::maxSystemTemp_c;
  }

  bool should_be_cleared() const override
  {
    // cleared below a threshold
    return hal::registers::read_CPU_temperature_degreesC() <= physical_constants::maxSystemTemp_c * 0.75;
  }

  /// Execution will lower the max brightness of the output
  void on_raised() const override
  {
    // limit brightness to half the max value
    constexpr brightness_t clampedBrightness =
            static_cast<brightness_t>(0.5 * ::lampda::brightness::absoluteMaximumBrightness);

    logic::brightness::set_max_brightness(clampedBrightness);
    logic::brightness::update_brightness(logic::brightness::get_saved_brightness());
    logic::brightness::update_saved_brightness();

    // this alert may lead to a shutdown, start phasing out the sunset timer
    // If the user decides to set the timer up again, it wont be lowered again by this alert
    logic::sunset_timer.shortcut_to_brightness_phase();
  }

  bool show() const override { return bsp::indicator::blink(300, 300, utils::ColorSpace::DARK_ORANGE); }

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
    return hal::registers::read_CPU_temperature_degreesC() >= physical_constants::criticalSystemTemp_c;
  }

  /// Shutdown fast
  uint32_t alert_shutdown_timeout() const override
  {
    // shutdown, critical temp is the absolute limit
    return 5000;
  }

  bool show() const override { return bsp::indicator::blink(100, 100, utils::ColorSpace::DARK_ORANGE); }

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
  static constexpr uint32_t frequency_on_ms = 1000;
  static constexpr uint32_t frequency_off_ms = 500;
  bool show() const override
  {
    return bsp::indicator::breeze(frequency_on_ms, frequency_off_ms, utils::ColorSpace::BLUE);
  }

  Type get_type() const override { return Type::BLUETOOTH_ADVERT; }

  bool should_be_cleared() const override
  {
    // cleared after a delay anyway
    return time_since_raised() > 60000;
  }
};

/**
 * \brief This alert is raised when a problem is detected in the hardware.
 * Usually, it means that an electrical component may have become unresponsive.
 */
struct Alert_HardwareAlert : public AlertBase
{
  bool show() const override
  {
    return bsp::indicator::blink(100, 100, {utils::ColorSpace::PURPLE, utils::ColorSpace::TEAL});
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
    return bsp::indicator::blink(100, 100, {utils::ColorSpace::WHITE, utils::ColorSpace::BLACK});
  }

  Type get_type() const override { return Type::CHARGER_ERROR; }
};

/**
 * \brief This alert is raised when new favorite mode is added.
 */
struct Alert_FavoriteSet : public AlertBase
{
  bool show() const override { return bsp::indicator::blink(100, 100, utils::ColorSpace::TEAL); }

  Type get_type() const override { return Type::FAVORITE_SET; }

  /// It is automatically cleared after a delay
  bool should_be_cleared() const override
  {
    // cleared after a delay
    return time_since_raised() > 1000;
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
    return bsp::indicator::blink(300, 200, {utils::ColorSpace::BLUE, utils::ColorSpace::YELLOW});
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
    return bsp::indicator::blink(100, 100, {utils::ColorSpace::PURPLE, utils::ColorSpace::WHITE});
  }

  Type get_type() const override { return Type::SYSTEM_OFF_FAILED; }

  /// Cleared after a delay
  bool should_be_cleared() const override
  {
    if (time_since_raised() > 2000)
    {
      // is this fails, the system is just too broken to be repaired
      hal::registers::enter_serial_dfu();
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
    return bsp::indicator::blink(100, 100, {utils::ColorSpace::PINK, utils::ColorSpace::ORANGE});
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
    return bsp::indicator::blink(
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

    return bsp::indicator::breeze(5000, 5000, buttonColor);
  }

  Type get_type() const override { return Type::SUNSET_TIMER_ENABLED; }
};

/**
 * \brief This alert is raised when the system starts without sleeping cleanly first.
 * It appears after updates, crashes, power failures, ...
 */
struct Alert_SkippedCleanSleep : public AlertBase
{
  bool show() const override { return bsp::indicator::blink(250, 250, utils::ColorSpace::PINK); }

  Type get_type() const override { return Type::SYSTEM_SLEEP_SKIPPED; }

  /// Auto cleared after a delay
  bool should_be_cleared() const override
  {
    // cleared after a delay
    return time_since_raised() > 3000;
  }
};

/**
 * \brief This alert is raised when a short circuit is detected in the USB lines.
 * The port should be cleaned before using it again.
 */
struct Alert_UsbPortShort : public AlertBase
{
  bool show() const override { return bsp::indicator::blink(100, 100, utils::ColorSpace::BLUE); }

  Type get_type() const override { return Type::USB_PORT_SHORT; }

  /// auto cleared after a delay
  bool should_be_cleared() const override { return time_since_raised() > 5000; }

  /// Prevent charge
  bool should_prevent_battery_charge() const override { return true; }
  /// Prevent USB use
  bool should_prevent_usb_port_use() const override { return true; }
};

/**
 * \brief This alert is raised when the output power goes out of the limits : overcurrent or overvoltage.
 * Clear on total reboot
 */
struct Alert_OtgOverLimits : public AlertBase
{
  bool show() const override
  {
    return bsp::indicator::blink(300, 200, {utils::ColorSpace::BLUE, utils::ColorSpace::YELLOW});
  }

  Type get_type() const override { return Type::OTG_OVER_LIMITS; }

  /// Block power output
  bool should_prevent_lamp_output() const override { return true; }
  /// Prevent USB use
  bool should_prevent_usb_port_use() const override { return true; }
};

// Init all alert objects
inline static Alert_SystemShutdownFailed alertSystemShutdownFailed {};
inline static Alert_BatteryMissing alertBatteryMissing {};
inline static Alert_HardwareAlert alertHardwareAlert {};
inline static Alert_TempCritical alertTempCritical {};
inline static Alert_UsbPortShort alertUsbPortShort {};
inline static Alert_SkippedCleanSleep alertSkippedCleanSleep {};
inline static Alert_OtgOverLimits alert_OtgOverLimits {};
inline static Alert_BatteryCritical alertBatteryCritical {};
inline static Alert_TempTooHigh alertTempTooHigh {};
inline static Alert_BatteryReadingIncoherent alertBatteryReadingIncoherent {};
inline static Alert_BatteryLow alertBatteryLow {};
inline static Alert_OtgFailed alertOtgFailed {};
inline static Alert_SystemInErrorState alertSystemInErrorState {};
inline static Alert_SystemInLockout alertSystemInLockout {};
inline static Alert_LongLoopUpdate alertLongLoopUpdate {};
inline static Alert_BluetoothAdvertisement alertBluetoothAdvertisement {};
inline static Alert_FavoriteSet alertFavoriteSet {};
inline static Alert_SunsetTimerSet alertSunsetTimerSet {};
inline static Alert_ChargerError alertChargerError {};

/// Alerts must be sorted by importance, only the first activated one will be shown
inline static AlertBase* const allAlerts[] = {
        &alertSystemShutdownFailed,
        &alertBatteryMissing, // if battery is missing, the system will also have the hardware alert
        &alertHardwareAlert,
        &alertTempCritical,
        &alertUsbPortShort,
        //
        &alertSkippedCleanSleep,
        // battery and temp related
        &alertBatteryCritical,
        &alertTempTooHigh,
        &alertBatteryReadingIncoherent,
        &alertBatteryLow,
        //
        &alert_OtgOverLimits,
        &alertOtgFailed,
        &alertSystemInErrorState,
        &alertSystemInLockout,

        // user side low priority alerts
        &alertLongLoopUpdate,
        &alertBluetoothAdvertisement,
        &alertFavoriteSet,
        &alertSunsetTimerSet,

        &alertChargerError,
};

void update_alerts()
{
  const uint32_t currTime = hal::time_ms();
  for (auto alert: allAlerts)
  {
    if (manager.is_raised(alert->get_type()))
    {
      // read before the decision: a raise() refreshing the alert meanwhile cancels the clear
      const uint32_t refreshCount = alert->refreshCount.load();
      if (alert->should_be_cleared())
      {
        manager.clear_unless_refreshed(alert->get_type(), alert->refreshCount, refreshCount);
      }
      else
      {
        // call this before any other processes, sets the delays
        if (alert->handle_raised_state(currTime))
        {
          // execute this alert action
          alert->on_raised();
        }
        // execute loop
        alert->execute_loop();

        if (alert->should_shutdown_system())
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
      }
      else if (alert->handle_lowered_state(currTime))
      {
        alert->on_lowered();
      }
    }
  }
}

void signal_wake_up_from_charger() { _startupChargerTime = hal::time_ms(); }

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
      bsp::indicator::set_color(utils::ColorSpace::BLACK);
      return;
    }

    // red to green, unless the user chose a color
    uint32_t overrideColor = 0;
    const bool hasOverrideColor = logic::indicator::get_color_override(overrideColor);
    const auto& batteryLevel = __internal::get_battery_level();
    const auto buttonColor =
            utils::ColorSpace::RGB(hasOverrideColor ? overrideColor :
                                                      utils::get_gradient(utils::ColorSpace::RED.get_rgb().color,
                                                                          utils::ColorSpace::GREEN.get_rgb().color,
                                                                          batteryLevel / 10000.0f));

    // display battery level
    const auto& chargerStatus = ::lampda::component::charger::get_state();
    if (!logic::power::is_in_output_mode() and chargerStatus.isInOtg)
    {
      bsp::indicator::breeze(500, 500, buttonColor);
    }
    else if (chargerStatus.is_charging())
    {
      // power detected with no charge or slow charging raises a special animation
      if (chargerStatus.status == ::lampda::component::charger::Charger_t::ChargerStatus_t::POWER_DETECTED or
          chargerStatus.status == ::lampda::component::charger::Charger_t::ChargerStatus_t::SLOW_CHARGING)
      {
        // fast blinking
        bsp::indicator::blink(500, 500, buttonColor);
      }
      // standard charge mode
      else
      {
        bsp::indicator::breeze(2000, 1000, buttonColor);
      }
    }
    // output mode, or end of charge : standard display
    else if (logic::power::is_in_output_mode() or chargerStatus.is_charge_finished())
    {
      // normal output mode
      // chargerStatus.isInOtg should be true
      bsp::indicator::set_color(buttonColor);
    }
    else
    {
      // we can handup here when starting/shutting down the system
      // no charger operation, no output mode
      bsp::indicator::blink(1000, 1000, buttonColor);
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
    bsp::indicator::blink(300, 300, utils::ColorSpace::WHITE);
  }
}

void show_all()
{
  if (manager.is_clear())
  {
    bsp::lampda_print("No alerts raised");
  }
  else
  {
    bsp::lampda_print("Raised alerts:");
    for (auto alert: allAlerts)
    {
      if (manager.is_raised(alert->get_type()))
      {
        bsp::lampda_print("- %s", AlertsToText(alert->get_type()));
      }
    }
  }
}

bool is_request_shutdown() { return _request_shutdown; }

void AlertManager_t::raise(const Type type)
{
  if (is_raised(type))
  {
    // The refresh and the clear decision of clear_unless_refreshed() hold the same lock: either the clear ran first
    // (the alert is not raised anymore, this raise is a new one), or it runs after the refresh, sees it and keeps the
    // alert. A refresh done here can not be cleared afterward.
    RaiseTimeLock lock;
    if (is_raised(type))
    {
      // update raise time
      for (auto& alert: allAlerts)
      {
        if (type == alert->get_type())
        {
          alert->update_raise_time_locked();
          break;
        }
      }
      return;
    }
  }

  // atomic: only the caller that actually set the bit signals the raise
  const uint32_t previous = _current.fetch_or(static_cast<uint32_t>(type));
  if ((previous & static_cast<uint32_t>(type)) != 0)
    return;

  statistics::signal_alert_raised(static_cast<uint32_t>(type));
  bsp::lampda_print("ALERT raised: %s", AlertsToText(type));
}

void AlertManager_t::clear(const Type type)
{
  // atomic, and an AND (not a XOR): a concurrent clear of the same bit cannot set it again
  const uint32_t previous = _current.fetch_and(~static_cast<uint32_t>(type));
  if ((previous & static_cast<uint32_t>(type)) == 0)
    return;
  bsp::lampda_print("ALERT cleared: %s", AlertsToText(type));
}

bool AlertManager_t::clear_unless_refreshed(const Type type,
                                            const std::atomic<uint32_t>& refreshCount,
                                            const uint32_t refreshCountAtDecision)
{
  const uint32_t bit = static_cast<uint32_t>(type);
  {
    // same lock as the refresh in raise(): no refresh can complete between the refresh check and the clear
    RaiseTimeLock lock;
    uint32_t expected = _current.load();
    while (true)
    {
      // already cleared by someone else
      if ((expected & bit) == 0)
        return false;
      // refreshed while the clear was decided: the alert is still active, keep it
      if (refreshCount.load() != refreshCountAtDecision)
        return false;
      // the bit is never removed when the clear is cancelled, so other threads never see a transient clear.
      // The CAS is still needed: the other bits of _current change without this lock
      if (_current.compare_exchange_weak(expected, expected & ~bit))
        break;
      // _current changed (or spurious failure): expected is reloaded, check again
    }
  }

  bsp::lampda_print("ALERT cleared: %s", AlertsToText(type));
  return true;
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
      return alert->time_since_raised();
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
    if (is_raised(alert->get_type()) && alert->should_prevent_lamp_output())
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
    if (is_raised(alert->get_type()) && alert->should_prevent_battery_charge())
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
    if (is_raised(alert->get_type()) && alert->should_prevent_usb_port_use())
    {
      return false;
    }
  }
  return true;
}

} // namespace alerts
} // namespace logic
} // namespace lampda

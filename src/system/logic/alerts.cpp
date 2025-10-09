#include "alerts.h"

#include "src/system/logic/statistics_handler.h"

#include "src/system/platform/time.h"
#include "src/system/platform/bluetooth.h"
#include "src/system/platform/registers.h"
#include "src/system/platform/print.h"

#include "src/system/physical/indicator.h"
#include "src/system/physical/battery.h"

#include "src/system/power/power_handler.h"

#include "src/system/utils/utils.h"
#include "src/system/utils/constants.h"
#include "src/system/utils/brightness_handle.h"
#include "src/system/utils/time_utils.h"

#include "src/system/power/charger.h"

namespace alerts {

AlertManager_t manager;
bool _request_shutdown = false;

// if true, do not display the indicator in normal mode
bool skipIndicator = false;

// stay at zero in normal operation
// set to the real startup time to ignore the battery alerts while the system starts
uint32_t _startupChargerTime = 0;
// ready to display battery alerts
bool is_battery_alert_ready()
{
  const bool isReady = _startupChargerTime == 0 or (time_ms() - _startupChargerTime) > 5000;
  if (isReady)
    _startupChargerTime = 0;
  return isReady;
}

inline const char* AlertsToText(const Type type)
{
  switch (type)
  {
    case MAIN_LOOP_FREEZE:
      return "MAIN_LOOP_FREEZE";
    case BATTERY_READINGS_INCOHERENT:
      return "BATTERY_READINGS_INCOHERENT";
    case BATTERY_CRITICAL:
      return "BATTERY_CRITICAL";
    case BATTERY_LOW:
      return "BATTERY_LOW";
    case LONG_LOOP_UPDATE:
      return "LONG_LOOP_UPDATE";
    case TEMP_TOO_HIGH:
      return "TEMP_TOO_HIGH";
    case TEMP_CRITICAL:
      return "TEMP_CRITICAL";
    case BLUETOOTH_ADVERT:
      return "BLUETOOTH_ADVERT";
    case HARDWARE_ALERT:
      return "HARDWARE_ALERT";
    case FAVORITE_SET:
      return "FAVORITE_SET";
    case OTG_FAILED:
      return "OTG_FAILED";
    case SYSTEM_OFF_FAILED:
      return "SYSTEM_OFF_FAILED";
    case SYSTEM_IN_ERROR_STATE:
      return "SYSTEM_IN_ERROR_STATE";
    case SYSTEM_IN_LOCKOUT:
      return "SYSTEM_IN_LOCKOUT";
    case SUNSET_TIMER_ENABLED:
      return "SUNSET_TIMER_ENABLED";
    case SYSTEM_SLEEP_SKIPPED:
      return "SYSTEM_SLEEP_SKIPPED";
    case USB_PORT_SHORT:
      return "USB_PORT_SHORT";
    default:
      return "UNSUPPORTED TYPE";
  }
}

namespace __internal {

// only sample battery level every N steps
uint16_t get_battery_level()
{
  static uint16_t lastPercent = 0;

  EVERY_N_MILLIS(1000.0)
  {
    const uint16_t newPercent = battery::get_battery_minimum_cell_level();
    if ((lastPercent / 100) != (newPercent / 100))
    {
      bluetooth::write_battery_level(newPercent / 100);
    }
    lastPercent = newPercent;
  }
  return lastPercent;
}

} // namespace __internal

struct AlertBase
{
  /**
   * \brief Return the timeout after which an alert will make the system auto-shutdown
   */
  virtual uint32_t alert_shutdown_timeout() const
  {
    // max timeout, will never be exeeded
    return UINT32_MAX;
  }

  /**
   * \brief function executed when this alert is raised
   */
  virtual void execute() const {};

  /**
   * \brief return true if this alert type should be raised
   */
  virtual bool should_be_raised() const { return false; }

  /**
   * \brief return true if this alert type should be cleared
   */
  virtual bool should_be_cleared() const { return false; }

  // display this alert
  virtual bool show() const { return indicator::blink(300, 300, utils::ColorSpace::WHITE); }

  virtual Type get_type() const = 0;

  virtual bool should_shutdown_system(const uint32_t time) final
  {
    return (time - raisedTime) > alert_shutdown_timeout();
  }

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

  // an alert continuously firing as to be updated
  void update_raise_time(const uint32_t time) { raisedTime = time; }

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

  // override to prevent lamp actions on alerts
  virtual bool should_prevent_lamp_output() const { return false; }
  virtual bool should_prevent_battery_charge() const { return false; }
  virtual bool should_prevent_usb_port_use() const { return false; }

  // private:
  //
  bool _isRaisedHandled = false;
  //
  bool _isLoweredHandled = false;
  //
  uint32_t raisedTime = 0;
  uint32_t loweredTime = 0;
};

struct Alert_MainLoopFreeze : public AlertBase
{
  Type get_type() const override { return Type::MAIN_LOOP_FREEZE; }
};

struct Alert_BatteryReadingIncoherent : public AlertBase
{
  bool show() const override { return indicator::blink(100, 100, {utils::ColorSpace::GREEN, utils::ColorSpace::RED}); }

  Type get_type() const override { return Type::BATTERY_READINGS_INCOHERENT; }

  bool should_be_cleared() const override
  {
    // cleared after a delay
    return raisedTime > 0 and (time_ms() - raisedTime) > 2000;
  }

  bool should_prevent_lamp_output() const override { return true; }
  bool should_prevent_battery_charge() const override { return true; }
  bool should_prevent_usb_port_use() const override { return true; }
};

struct Alert_BatteryCritical : public AlertBase
{
  bool should_be_raised() const override
  {
    if (not is_battery_alert_ready())
      return false;
    const auto& chargerState = charger::get_state();
    return not chargerState.is_effectivly_charging() and __internal::get_battery_level() < batteryCritical;
  }

  bool should_be_cleared() const override
  {
    // battery low can only be cleared on charging operations
    const auto& chargerState = charger::get_state();
    return chargerState.is_effectivly_charging();
  }

  uint32_t alert_shutdown_timeout() const override
  {
    // shutdown after 2 seconds
    return 2000;
  }

  bool show() const override
  {
    // fast blink red
    return indicator::blink(100, 100, utils::ColorSpace::RED);
  }

  Type get_type() const override { return Type::BATTERY_CRITICAL; }
};

struct Alert_BatteryLow : public AlertBase
{
  bool should_be_raised() const override
  {
    if (not is_battery_alert_ready())
      return false;

    const auto& chargerState = charger::get_state();
    const auto& batteryLevel = __internal::get_battery_level();
    const bool isBatteryLow = not chargerState.is_effectivly_charging() and batteryLevel < batteryLow;
    // battery low will be raise, notify bluetooth
    if (isBatteryLow)
      bluetooth::notify_battery_level(batteryLevel / 100);
    return isBatteryLow;
  }

  bool should_be_cleared() const override
  {
    // battery low can only be cleared on charging operations
    const auto& chargerState = charger::get_state();
    return chargerState.is_effectivly_charging();
  }

  void execute() const override
  {
    // limit brightness to quarter of the max value
    constexpr brightness_t clampedBrightness = 0.25 * maxBrightness;

    // save some battery
    bluetooth::stop_bluetooth_advertising();

    brightness::set_max_brightness(clampedBrightness);
    brightness::update_brightness(brightness::get_brightness());
  }

  bool show() const override
  {
    // fast blink red
    return indicator::blink(300, 300, utils::ColorSpace::RED);
  }

  Type get_type() const override { return Type::BATTERY_LOW; }
};

struct Alert_LongLoopUpdate : public AlertBase
{
  bool show() const override { return indicator::blink(400, 400, utils::ColorSpace::FUSHIA); }

  Type get_type() const override { return Type::LONG_LOOP_UPDATE; }
};

struct Alert_TempTooHigh : public AlertBase
{
  bool should_be_raised() const override
  {
    // raised above a threshold
    return read_CPU_temperature_degreesC() >= maxSystemTemp_c;
  }

  bool should_be_cleared() const override
  {
    // cleared below a threshold
    return read_CPU_temperature_degreesC() <= maxSystemTemp_c * 0.75;
  }

  void execute() const override
  {
    // limit brightness to half the max value
    constexpr brightness_t clampedBrightness = 0.5 * maxBrightness;

    brightness::set_max_brightness(clampedBrightness);
    brightness::update_brightness(brightness::get_brightness());
  }

  bool show() const override { return indicator::blink(300, 300, utils::ColorSpace::DARK_ORANGE); }

  Type get_type() const override { return Type::TEMP_TOO_HIGH; }
};

struct Alert_TempCritical : public AlertBase
{
  bool should_be_raised() const override
  {
    // raised above a threshold
    return read_CPU_temperature_degreesC() >= criticalSystemTemp_c;
  }

  // never need to clear this alert, temp too high will always be a complete shutdown
  // bool should_be_cleared() const;

  uint32_t alert_shutdown_timeout() const
  {
    // shutdown, critical temp is the absolute limit
    return 5000;
  }

  bool show() const override { return indicator::blink(100, 100, utils::ColorSpace::DARK_ORANGE); }

  Type get_type() const override { return Type::TEMP_CRITICAL; }

  bool should_prevent_lamp_output() const override { return true; }
  bool should_prevent_battery_charge() const override { return true; }
  bool should_prevent_usb_port_use() const override { return true; }
};

struct Alert_BluetoothAdvertisement : public AlertBase
{
  bool show() const override { return indicator::breeze(1000, 500, utils::ColorSpace::BLUE); }

  Type get_type() const override { return Type::BLUETOOTH_ADVERT; }
};

struct Alert_HardwareAlert : public AlertBase
{
  bool show() const override
  {
    return indicator::blink(100, 100, {utils::ColorSpace::PURPLE, utils::ColorSpace::TEAL});
  }

  Type get_type() const override { return Type::HARDWARE_ALERT; }

  bool should_prevent_lamp_output() const override { return true; }
  bool should_prevent_battery_charge() const override { return true; }
  bool should_prevent_usb_port_use() const override { return true; }
};

struct Alert_FavoriteSet : public AlertBase
{
  bool show() const override { return indicator::blink(100, 100, utils::ColorSpace::TEAL); }

  Type get_type() const override { return Type::FAVORITE_SET; }

  bool should_be_cleared() const override
  {
    // cleared after a delay
    return raisedTime > 0 and (time_ms() - raisedTime) > 1000;
  }
};

struct Alert_OtgFailed : public AlertBase
{
  bool show() const override
  {
    return indicator::blink(300, 200, {utils::ColorSpace::BLUE, utils::ColorSpace::YELLOW});
  }

  Type get_type() const override { return Type::OTG_FAILED; }

  bool should_prevent_lamp_output() const override { return true; }
};

struct Alert_SystemShutdownFailed : public AlertBase
{
  bool show() const override
  {
    return indicator::blink(100, 100, {utils::ColorSpace::PURPLE, utils::ColorSpace::WHITE});
  }

  Type get_type() const override { return Type::SYSTEM_OFF_FAILED; }

  bool should_be_cleared() const override
  {
    // cleared after a delay
    if (raisedTime > 0 and (time_ms() - raisedTime) > 2000)
    {
      // is this fails, the system is just too broken to be repaired
      enter_serial_dfu();
      return true;
    }
    return false;
  }

  bool should_prevent_lamp_output() const override { return true; }
  bool should_prevent_battery_charge() const override { return true; }
  bool should_prevent_usb_port_use() const override { return true; }
};

struct Alert_SystemInErrorState : public AlertBase
{
  bool show() const override
  {
    return indicator::blink(100, 100, {utils::ColorSpace::PINK, utils::ColorSpace::ORANGE});
  }

  Type get_type() const override { return Type::SYSTEM_IN_ERROR_STATE; }

  bool should_prevent_lamp_output() const override { return true; }
  bool should_prevent_battery_charge() const override { return true; }
  bool should_prevent_usb_port_use() const override { return true; }
};

struct Alert_SystemInLockout : public AlertBase
{
  bool show() const override
  {
    return indicator::blink(
            200,
            50,
            {utils::ColorSpace::BLACK, utils::ColorSpace::BLUE, utils::ColorSpace::WHITE, utils::ColorSpace::RED});
  }

  Type get_type() const override { return Type::SYSTEM_IN_LOCKOUT; }
};

struct Alert_SunsetTimerSet : public AlertBase
{
  bool show() const override
  {
    // red to green
    const auto buttonColor =
            utils::ColorSpace::RGB(utils::get_gradient(utils::ColorSpace::RED.get_rgb().color,
                                                       utils::ColorSpace::GREEN.get_rgb().color,
                                                       battery::get_battery_minimum_cell_level() / 10000.0));

    return indicator::breeze(5000, 5000, buttonColor);
  }

  Type get_type() const override { return Type::SUNSET_TIMER_ENABLED; }
};

struct Alert_SkippedCleanSleep : public AlertBase
{
  bool show() const override { return indicator::blink(250, 250, utils::ColorSpace::PINK); }

  Type get_type() const override { return Type::SYSTEM_SLEEP_SKIPPED; }

  bool should_be_cleared() const override
  {
    // cleared after a delay
    return (raisedTime > 0 and (time_ms() - raisedTime) > 3000);
  }
};

struct Alert_UsbPortShort : public AlertBase
{
  bool show() const override { return indicator::blink(100, 100, utils::ColorSpace::BLUE); }

  Type get_type() const override { return Type::USB_PORT_SHORT; }

  bool should_be_cleared() const override
  {
    // auto cleared after a delay
    return (raisedTime > 0 and (time_ms() - raisedTime) > 5000);
  }

  // prevent charge
  bool should_prevent_battery_charge() const override { return true; }
  bool should_prevent_usb_port_use() const override { return true; }
};

// Alerts must be sorted by importance, only the first activated one will be shown
AlertBase* allAlerts[] = {new Alert_SystemShutdownFailed,
                          new Alert_HardwareAlert,
                          new Alert_TempCritical,
                          new Alert_UsbPortShort,
                          //
                          new Alert_SkippedCleanSleep,
                          new Alert_BatteryReadingIncoherent,
                          // battery and temp related
                          new Alert_TempTooHigh,
                          new Alert_BatteryCritical,
                          new Alert_BatteryLow,
                          //
                          new Alert_OtgFailed,
                          new Alert_SystemInErrorState,
                          new Alert_SystemInLockout,
                          // user side low priority alerts
                          new Alert_LongLoopUpdate,
                          new Alert_BluetoothAdvertisement,
                          new Alert_FavoriteSet,
                          new Alert_SunsetTimerSet};

void update_alerts()
{
  const uint32_t currTime = time_ms();
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
        lampda_print("Raised alert %s", AlertsToText(alert->get_type()));
      }
      else
      {
        alert->handle_lowered_state(currTime);
      }
    }
  }
}

void signal_wake_up_from_charger() { _startupChargerTime = time_ms(); }

void handle_all(const bool shouldIgnoreAlerts)
{
  // update all alerts
  update_alerts();

  if (shouldIgnoreAlerts or manager.is_clear())
  {
    // no alerts: reset the max brightness
    brightness::set_max_brightness(maxBrightness);

    // do nothing to display anything
    if (skipIndicator)
    {
      indicator::set_color(utils::ColorSpace::BLACK);
      return;
    }

    // red to green
    const auto buttonColor =
            utils::ColorSpace::RGB(utils::get_gradient(utils::ColorSpace::RED.get_rgb().color,
                                                       utils::ColorSpace::GREEN.get_rgb().color,
                                                       battery::get_battery_minimum_cell_level() / 10000.0));

    // display battery level
    const auto& chargerStatus = charger::get_state();
    if (!power::is_in_output_mode() and chargerStatus.isInOtg)
    {
      indicator::breeze(500, 500, buttonColor);
    }
    else if (chargerStatus.is_charging())
    {
      // power detected with no charge or slow charging raises a special animation
      if (chargerStatus.status == charger::Charger_t::ChargerStatus_t::POWER_DETECTED or
          chargerStatus.status == charger::Charger_t::ChargerStatus_t::SLOW_CHARGING)
      {
        // fast blinking
        indicator::blink(500, 500, buttonColor);
      }
      // standard charge mode
      else
      {
        indicator::breeze(2000, 1000, buttonColor);
      }
    }
    // output mode, or end of charge : standard display
    else if (power::is_in_output_mode() or chargerStatus.is_charge_finished())
    {
      // normal output mode
      // chargerStatus.isInOtg should be true
      indicator::set_color(buttonColor);
    }
    else
    {
      // we can handup here when starting/shutting down the system

      // red to green
      const auto buttonColor =
              utils::ColorSpace::RGB(utils::get_gradient(utils::ColorSpace::RED.get_rgb().color,
                                                         utils::ColorSpace::GREEN.get_rgb().color,
                                                         battery::get_battery_minimum_cell_level() / 10000.0));

      // no charger operation, no output mode
      indicator::blink(1000, 1000, buttonColor);
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
    indicator::blink(300, 300, utils::ColorSpace::WHITE);
  }
}

void show_all()
{
  if (manager.is_clear())
  {
    lampda_print("No alerts raised");
  }
  else
  {
    lampda_print("Raised alerts:");
    for (auto alert: allAlerts)
    {
      if (manager.is_raised(alert->get_type()))
      {
        lampda_print("- %s", AlertsToText(alert->get_type()));
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
        alert->update_raise_time(time_ms());
        break;
      }
    }
    return;
  }
  else
  {
    statistics::signal_alert_raised(type);
  }

  lampda_print("ALERT raised: %s", AlertsToText(type));
  _current |= type;
}

void AlertManager_t::clear(const Type type)
{
  if (not is_raised(type))
    return;
  lampda_print("ALERT cleared: %s", AlertsToText(type));
  _current ^= type;
}

uint32_t AlertManager_t::get_time_since_raised(const Type type)
{
  for (auto alert: allAlerts)
  {
    if (alert->get_type() == type)
    {
      // safety if the alert was raised in between, or not yet handled
      if (not is_raised(type) or not alert->_isRaisedHandled)
        return 0;
      // compute raised time
      return time_ms() - alert->raisedTime;
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

namespace indicator {

static inline uint8_t _level = 0;

void set_brightness_level(const uint8_t level)
{
  static constexpr uint8_t lowBrightness = 64;
  static constexpr uint8_t midBrightness = 128;
  switch (level)
  {
    case 1:
      _level = 1;
      alerts::skipIndicator = true;
      indicator::set_brightness(lowBrightness);
      break;
    case 2:
      _level = 2;
      alerts::skipIndicator = false;
      indicator::set_brightness(lowBrightness);
      break;
    // 0 and default are the same : level too high should loop back
    case 0:
    default:
      _level = 0;
      alerts::skipIndicator = false;
      indicator::set_brightness(255);
      break;
  }
}

uint8_t get_brightness_level() { return _level; }

} // namespace indicator

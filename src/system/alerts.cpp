#include "alerts.h"

#include "src/system/platform/time.h"
#include "src/system/platform/bluetooth.h"
#include "src/system/platform/registers.h"

#include "src/system/physical/indicator.h"
#include "src/system/physical/battery.h"

#include "src/system/utils/print.h"
#include "src/system/utils/utils.h"
#include "src/system/utils/constants.h"
#include "src/system/utils/brightness_handle.h"

#include "src/system/charger/charger.h"

namespace alerts {

AlertManager_t manager;
bool _request_shutdown = false;

inline const char* AlertsToText(const Type type)
{
  switch (type)
  {
    case MAIN_LOOP_FREEZE:
      return "MAIN_LOOP_FREEZE";
    case BATTERY_READINGS_INCOHERENT:
      return "BATTERY_READING_INCOHERENT";
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
    case OTG_ACTIVATED:
      return "OTG_ACTIVATED";
    case OTG_FAILED:
      return "OTG_FAILED";
    default:
      return "UNSUPPORTED TYPE";
  }
}

void AlertManager_t::raise(const Type type)
{
  if (is_raised(type))
    return;

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

namespace __internal {

// only sample battery level every N steps
uint16_t get_battery_level()
{
  static uint32_t lastCallTime = 0;
  static uint16_t lastPercent = 0;

  const uint32_t currTime = time_ms();
  if (lastCallTime == 0 or (currTime - lastCallTime) > 1000.0)
  {
    lastCallTime = currTime;
    lastPercent = battery::get_battery_level();
  }
  return lastPercent;
}

// only sample proc temperature level every N steps
float get_processor_temperature()
{
  static uint32_t lastCallTime = 0;
  static float procTemp = 0.0;

  const uint32_t currTime = time_ms();
  if (lastCallTime == 0 or (currTime - lastCallTime) > 500.0)
  {
    lastCallTime = currTime;
    procTemp = read_CPU_temperature_degreesC();
  }
  return procTemp;
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
  bool show() const override { return indicator::blink(100, 100, utils::ColorSpace::GREEN); }

  Type get_type() const override { return Type::BATTERY_READINGS_INCOHERENT; }
};

struct Alert_BatteryCritical : public AlertBase
{
  bool should_be_raised() const override
  {
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
    const auto& chargerState = charger::get_state();
    return not chargerState.is_effectivly_charging() and __internal::get_battery_level() < batteryLow;
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
    bluetooth::disable_bluetooth();

    brightness::set_max_brightness(clampedBrightness);
    brightness::update_brightness(brightness::get_brightness());
    brightness::update_previous_brightness();
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
    return __internal::get_processor_temperature() >= maxSystemTemp_c;
  }

  bool should_be_cleared() const override
  {
    // cleared below a threshold
    return __internal::get_processor_temperature() <= maxSystemTemp_c * 0.75;
  }

  void execute() const override
  {
    // limit brightness to half the max value
    constexpr brightness_t clampedBrightness = 0.5 * maxBrightness;

    brightness::set_max_brightness(clampedBrightness);
    brightness::update_brightness(brightness::get_brightness());
    brightness::update_previous_brightness();
  }

  bool show() const override { return indicator::blink(300, 300, utils::ColorSpace::ORANGE); }

  Type get_type() const override { return Type::TEMP_TOO_HIGH; }
};

struct Alert_TempCritical : public AlertBase
{
  bool should_be_raised() const override
  {
    // raised above a threshold
    return __internal::get_processor_temperature() >= criticalSystemTemp_c;
  }

  // never need to clear this alert, temp too high will always be a complete shutdown
  // bool should_be_cleared() const;

  uint32_t alert_shutdown_timeout() const
  {
    // shutdown, critical temp is the absolute limit
    return 0;
  }

  bool show() const override { return indicator::blink(100, 100, utils::ColorSpace::ORANGE); }

  Type get_type() const override { return Type::TEMP_CRITICAL; }
};

struct Alert_BluetoothAdvertisement : public AlertBase
{
  bool show() const override { return indicator::breeze(1000, 500, utils::ColorSpace::BLUE); }

  Type get_type() const override { return Type::BLUETOOTH_ADVERT; }
};

struct Alert_HardwareAlert : public AlertBase
{
  bool show() const override { return indicator::blink(100, 100, utils::ColorSpace::GREEN); }

  Type get_type() const override { return Type::HARDWARE_ALERT; }
};

struct Alert_OtgActivated : public AlertBase
{
  bool show() const override
  {
    // red to green
    const auto buttonColor = utils::ColorSpace::RGB(utils::get_gradient(utils::ColorSpace::RED.get_rgb().color,
                                                                        utils::ColorSpace::GREEN.get_rgb().color,
                                                                        battery::get_battery_level() / 10000.0));

    return indicator::breeze(500, 500, buttonColor);
  }

  Type get_type() const override { return Type::OTG_ACTIVATED; }
};

struct Alert_OtgFailed : public AlertBase
{
  bool show() const override { return indicator::blink(200, 200, utils::ColorSpace::FUSHIA); }

  Type get_type() const override { return Type::OTG_FAILED; }
};

// Alerts must be sorted by importance, only the first activated one will be shown
AlertBase* allAlerts[] = {new Alert_HardwareAlert,
                          new Alert_TempCritical,
                          new Alert_TempTooHigh,
                          new Alert_BatteryReadingIncoherent,
                          new Alert_BatteryCritical,
                          new Alert_BatteryLow,
                          new Alert_LongLoopUpdate,
                          new Alert_BluetoothAdvertisement,
                          new Alert_OtgFailed,
                          new Alert_OtgActivated};

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
      }
      else
      {
        alert->handle_lowered_state(currTime);
      }
    }
  }
}

void handle_all(const bool shouldIgnoreAlerts)
{
  // update all alerts
  update_alerts();

  if (shouldIgnoreAlerts or manager.is_clear())
  {
    brightness::set_max_brightness(maxBrightness); // no alerts: reset the max brightness

    // red to green
    const auto buttonColor = utils::ColorSpace::RGB(utils::get_gradient(utils::ColorSpace::RED.get_rgb().color,
                                                                        utils::ColorSpace::GREEN.get_rgb().color,
                                                                        battery::get_battery_level() / 10000.0));

    // display battery level
    const auto& chargerStatus = charger::get_state();
    if (chargerStatus.is_charging())
    {
      // power detected with no charge or slow charging raises a special animation
      if (chargerStatus.status == charger::Charger_t::ChargerStatus_t::POWER_DETECTED or
          chargerStatus.status == charger::Charger_t::ChargerStatus_t::SLOW_CHARGING)
      {
        // fast blinking
        // TODO: find a better way to tell user that the chargeur is bad
        indicator::blink(500, 500, buttonColor);
      }
      // standard charge mode
      else
      {
        indicator::breeze(2000, 1000, buttonColor);
      }
    }
    else
    {
      // normal mode
      indicator::set_color(buttonColor);
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

} // namespace alerts
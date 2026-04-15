#include "statistics_handler.h"

#include "src/system/platform/time.h"
#include "src/system/platform/print.h"

#include "src/system/physical/fileSystem.h"

#include "src/system/utils/utils.h"
#include <array>
#include <cstdint>

namespace lampda {
namespace logic {
namespace statistics {

/*
 *
 * Keep track of lamp use statistics
 * - button press
 * - time turn on (in minutes)
 * - time output on (in minutes)
 * - time battery charging (in minutes)
 * - alerts raised count
 *
 * Fill more as needed
 */

/// Size of the alert array. It should be egal to the maximum alert count
static constexpr uint8_t alertArraySize = 32;

/**
 * \brief Store the statistics of the system.
 */
struct Statistics_t
{
  /// number of timed powered on
  uint32_t boot_count = 0;
  /// Number of button presses
  uint32_t button_press_count = 0;

  // About 136 years can be used here, ok :)
  // UINT32_MAX / M  / H  / D  / Y
  // 4294967295 / 60 / 60 / 24 / 365 = 136.1925 years
  // in seconds, it would be 2.2 years, which may be short for this system

  /// Count the total minutes that the circuit was powered on.
  /// Should comparable to output_on_minutes + battery_charge_minutes
  uint32_t system_on_minutes = 0;
  /// Count the total minutes that the output has been on
  uint32_t output_on_minutes = 0;
  /// Count the total minutes that the battery has been charging
  uint32_t battery_charge_minutes = 0;

  /// Keep track of the number of times a target alert was raised
  std::array<uint32_t, alertArraySize> alertRaisedCnt = {};
};

/// Statistics holder
Statistics_t statistics;

static constexpr uint32_t bootCountKey = utils::hash("bootCnt");        ///< hash key of the bootCount statistic
static constexpr uint32_t buttonPressCountKey = utils::hash("buttonP"); ///< hash key of the button press count
static constexpr uint32_t systemOnTimeSKey = utils::hash("sysOn");      ///< hash key of the system on time
static constexpr uint32_t outputOnTimeSKey = utils::hash("outOn");      ///< hash key of the output on time
static constexpr uint32_t chargeOnTimeSKey = utils::hash("chrgOn");     ///< hash key of the charging time

/// compute a key for each alert
uint32_t get_alert_storage_key(uint8_t alertIndex) { return 0xFFFFFF00 | alertIndex; }

/// system on time is easy: system always starts with time zero
uint32_t get_system_on_time() { return statistics.system_on_minutes + max<uint32_t>(1, platform::time_s() / 60); }

void load_from_memory()
{
  physical::fileSystem::system::get_value(bootCountKey, statistics.boot_count);
  statistics.boot_count += 1;

  physical::fileSystem::system::get_value(buttonPressCountKey, statistics.button_press_count);
  physical::fileSystem::system::get_value(systemOnTimeSKey, statistics.system_on_minutes);
  physical::fileSystem::system::get_value(outputOnTimeSKey, statistics.output_on_minutes);
  physical::fileSystem::system::get_value(chargeOnTimeSKey, statistics.battery_charge_minutes);

  for (uint8_t alertIndex = 0; alertIndex < alertArraySize; alertIndex++)
  {
    const uint32_t alertCntBefore = statistics.alertRaisedCnt[alertIndex];
    physical::fileSystem::system::get_value(get_alert_storage_key(alertIndex), statistics.alertRaisedCnt[alertIndex]);
    if (alertCntBefore > statistics.alertRaisedCnt[alertIndex])
    {
      // fallback to prevent early alert crush
      statistics.alertRaisedCnt[alertIndex] = alertCntBefore;
    }
  }
}

void write_to_memory()
{
  physical::fileSystem::system::set_value(bootCountKey, statistics.boot_count);
  physical::fileSystem::system::set_value(buttonPressCountKey, statistics.button_press_count);
  physical::fileSystem::system::set_value(outputOnTimeSKey, statistics.output_on_minutes);
  physical::fileSystem::system::set_value(chargeOnTimeSKey, statistics.battery_charge_minutes);

  // system time starts at zero
  physical::fileSystem::system::set_value(systemOnTimeSKey, get_system_on_time());

  // store alerts
  for (uint8_t alertIndex = 0; alertIndex < alertArraySize; alertIndex++)
  {
    physical::fileSystem::system::set_value(get_alert_storage_key(alertIndex), statistics.alertRaisedCnt[alertIndex]);
  }
}

void signal_button_press() { statistics.button_press_count += 1; }

/// Keep track of the output on start time
inline static uint32_t outputOn_time_s = UINT32_MAX;

/**
 * \brief Return the sum of the output on time.
 * Depend on outputOn_time_s correct initialization.
 */
uint32_t get_output_on_time()
{
  if (outputOn_time_s != UINT32_MAX)
  {
    const uint32_t currentTime_s = platform::time_s();
    if (currentTime_s >= outputOn_time_s)
      return statistics.output_on_minutes + max<uint32_t>(1, (currentTime_s - outputOn_time_s) / 60);
    else
      // round up to the minute
      return statistics.output_on_minutes + 1;
  }
  return statistics.output_on_minutes;
}

void signal_output_on()
{
  if (outputOn_time_s == UINT32_MAX)
    outputOn_time_s = platform::time_s();
}

void signal_output_off()
{
  statistics.output_on_minutes = get_output_on_time();
  outputOn_time_s = UINT32_MAX;
}

/// store the battery charge time sum
inline static uint32_t batteryCharge_time_s = UINT32_MAX;
/**
 * \brief Get the total battery charge time.
 * Uses internally the batteryCharge_time_s to track when the charge started.
 */
uint32_t get_battery_charging_time()
{
  if (batteryCharge_time_s != UINT32_MAX)
  {
    const uint32_t currentTime_s = platform::time_s();
    if (currentTime_s >= batteryCharge_time_s)
      return statistics.battery_charge_minutes + max<uint32_t>(1, (currentTime_s - batteryCharge_time_s) / 60);
    else
      // round up to the next minute
      return statistics.battery_charge_minutes + 1;
  }
  return statistics.battery_charge_minutes;
}

void signal_battery_charging_on()
{
  if (batteryCharge_time_s == UINT32_MAX)
    batteryCharge_time_s = platform::time_s();
}

void signal_battery_charging_off()
{
  //
  statistics.battery_charge_minutes = get_battery_charging_time();
  batteryCharge_time_s = UINT32_MAX;
}

void signal_alert_raised(uint32_t alertMask)
{
  uint8_t alertIndex = 0;
  while (alertMask != 0)
  {
    alertMask >>= 1;
    alertIndex++;
  }
  if (alertIndex != 0 and alertIndex < alertArraySize)
    statistics.alertRaisedCnt[alertIndex - 1] += 1;
}

void show(const bool shouldShowAlerts)
{
  platform::lampda_print(
          "stats:\n"
          "boot cnt: %u\n"
          "button clicks: %u\n"
          "time on %umin\n"
          "output on %umin\n"
          "charge on %umin",
          statistics.boot_count,
          statistics.button_press_count,
          get_system_on_time(),
          get_output_on_time(),
          get_battery_charging_time());

  if (shouldShowAlerts)
  {
    platform::lampda_print("alert raised cnt stats :");
    bool anyAlertToDisplay = false;
    // store alerts
    for (uint8_t alertIndex = 0; alertIndex < alertArraySize; alertIndex++)
    {
      if (statistics.alertRaisedCnt[alertIndex] > 0)
      {
        anyAlertToDisplay = true;
        platform::lampda_print("- %u : %u", alertIndex, statistics.alertRaisedCnt[alertIndex]);
      }
    }
    if (not anyAlertToDisplay)
      platform::lampda_print("no alerts registered");
  }
}

} // namespace statistics
} // namespace logic
} // namespace lampda

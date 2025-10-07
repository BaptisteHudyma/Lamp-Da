#include "statistics_handler.h"

#include "src/system/platform/time.h"
#include "src/system/platform/print.h"

#include "src/system/physical/fileSystem.h"

#include "src/system/utils/utils.h"
#include <cstdint>

namespace statistics {

/**
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

//
inline static uint32_t boot_count = 0;
inline static uint32_t button_press_count = 0;
// About 136 years can be used here, ok :)
// UINT32_MAX / M  / H  / D  / Y
// 4294967295 / 60 / 60 / 24 / 365 = 136.1925 years
// in seconds, it would be 2.2 years, which may be short for this system
inline static uint32_t system_on_minutes = 0;
inline static uint32_t output_on_minutes = 0;
inline static uint32_t battery_charge_minutes = 0;

static constexpr uint8_t alertArraySize = 32;
inline static uint32_t alertRaisedCnt[alertArraySize] = {};

static constexpr uint32_t bootCountKey = utils::hash("bootCnt");
static constexpr uint32_t buttonPressCountKey = utils::hash("buttonP");
static constexpr uint32_t systemOnTimeSKey = utils::hash("sysOn");
static constexpr uint32_t outputOnTimeSKey = utils::hash("outOn");
static constexpr uint32_t chargeOnTimeSKey = utils::hash("chrgOn");

// compute a key for each alert
uint32_t get_alert_storage_key(uint8_t alertIndex) { return 0xFFFFFF00 | alertIndex; }

// system on time is easy: system always starts with time zero
uint32_t get_system_on_time() { return system_on_minutes + max(1, time_s() / 60); }

/**
 */
void load_from_memory()
{
  fileSystem::system::get_value(bootCountKey, boot_count);
  boot_count += 1;

  fileSystem::system::get_value(buttonPressCountKey, button_press_count);
  fileSystem::system::get_value(systemOnTimeSKey, system_on_minutes);
  fileSystem::system::get_value(outputOnTimeSKey, output_on_minutes);
  fileSystem::system::get_value(chargeOnTimeSKey, battery_charge_minutes);

  for (uint8_t alertIndex = 0; alertIndex < alertArraySize; alertIndex++)
  {
    const uint32_t alertCntBefore = alertRaisedCnt[alertIndex];
    fileSystem::system::get_value(get_alert_storage_key(alertIndex), alertRaisedCnt[alertIndex]);
    if (alertCntBefore > alertRaisedCnt[alertIndex])
    {
      // fallback to prevent early alert crush
      alertRaisedCnt[alertIndex] = alertCntBefore;
    }
  }
}

/**
 */
void write_to_memory()
{
  fileSystem::system::set_value(bootCountKey, boot_count);
  fileSystem::system::set_value(buttonPressCountKey, button_press_count);
  fileSystem::system::set_value(outputOnTimeSKey, output_on_minutes);
  fileSystem::system::set_value(chargeOnTimeSKey, battery_charge_minutes);

  // system time starts at zero
  fileSystem::system::set_value(systemOnTimeSKey, get_system_on_time());

  // store alerts
  for (uint8_t alertIndex = 0; alertIndex < alertArraySize; alertIndex++)
  {
    fileSystem::system::set_value(get_alert_storage_key(alertIndex), alertRaisedCnt[alertIndex]);
  }
}

/**
 */
void signal_button_press() { button_press_count += 1; }

/**
 */
inline static uint32_t outputOn_time_s = UINT32_MAX;
uint32_t get_output_on_time()
{
  if (outputOn_time_s != UINT32_MAX)
  {
    const uint32_t currentTime_s = time_s();
    if (currentTime_s >= outputOn_time_s)
      return output_on_minutes + max(1, (currentTime_s - outputOn_time_s) / 60);
    else
      // round up to the minute
      return output_on_minutes + 1;
  }
  return output_on_minutes;
}

void signal_output_on()
{
  if (outputOn_time_s == UINT32_MAX)
    outputOn_time_s = time_s();
}

void signal_output_off()
{
  output_on_minutes = get_output_on_time();
  outputOn_time_s = UINT32_MAX;
}

/**
 */
inline static uint32_t batteryCharge_time_s = UINT32_MAX;
uint32_t get_battery_charging_time()
{
  if (batteryCharge_time_s != UINT32_MAX)
  {
    const uint32_t currentTime_s = time_s();
    if (currentTime_s >= batteryCharge_time_s)
      return battery_charge_minutes + max(1, (currentTime_s - batteryCharge_time_s) / 60);
    else
      // round up to the next minute
      return battery_charge_minutes + 1;
  }
  return battery_charge_minutes;
}

void signal_battery_charging_on()
{
  if (batteryCharge_time_s == UINT32_MAX)
    batteryCharge_time_s = time_s();
}
void signal_battery_charging_off()
{
  //
  battery_charge_minutes = get_battery_charging_time();
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
    alertRaisedCnt[alertIndex - 1] += 1;
}

void show(const bool shouldShowAlerts)
{
  lampda_print(
          "stats:\n"
          "boot cnt: %u\n"
          "button clicks: %u\n"
          "time on %umin\n"
          "output on %umin\n"
          "charge on %umin",
          boot_count,
          button_press_count,
          get_system_on_time(),
          get_output_on_time(),
          get_battery_charging_time());

  if (shouldShowAlerts)
  {
    lampda_print("alert raised cnt stats :");
    bool anyAlertToDisplay = false;
    // store alerts
    for (uint8_t alertIndex = 0; alertIndex < alertArraySize; alertIndex++)
    {
      if (alertRaisedCnt[alertIndex] > 0)
      {
        anyAlertToDisplay = true;
        lampda_print("- %u : %u", alertIndex, alertRaisedCnt[alertIndex]);
      }
    }
    if (not anyAlertToDisplay)
      lampda_print("no alerts registered");
  }
}

} // namespace statistics

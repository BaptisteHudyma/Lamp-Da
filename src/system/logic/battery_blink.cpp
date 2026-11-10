#include "battery_blink.h"

#include "src/system/hal/time.h"

#include "src/system/bsp/balancer.h"
#include "src/system/bsp/text_out.h"

#include "src/system/component/battery.h"
#include "src/system/component/button.h"
#include "src/system/component/charger.h"
#include "src/system/component/output_power.h"

#include "src/system/logic/alerts.h"

#include "src/system/common/user_commands.h"

#include "src/user/functions.h"

namespace lampda {
namespace logic {
namespace battery_blink {

void BlinkTracker::reset(const uint32_t time_ms)
{
  _outputStartTime_ms = time_ms;
  _candidateStartTime_ms = time_ms;
  _candidateCount = 0;
  _displayedCount = 0;
  _blinksLeft = 0;
  _hasReferenceLevel = false;
}

bool BlinkTracker::update(const uint32_t time_ms, const uint16_t level, const bool isLevelValid, const bool canBlink)
{
  const uint8_t count = isLevelValid ? get_blink_count(level) : 0;
  if (count != _candidateCount)
  {
    _candidateCount = count;
    _candidateStartTime_ms = time_ms;
  }

  const bool isUsedLongEnough = time_ms - _outputStartTime_ms >= minOutputUseTime_ms;
  const bool isFiltered = time_ms - _candidateStartTime_ms >= levelFilterWindow_ms;
  if (isLevelValid and isUsedLongEnough and isFiltered)
  {
    // thresholds passed during the use time are never displayed
    if (not _hasReferenceLevel)
    {
      _hasReferenceLevel = true;
      _displayedCount = _candidateCount;
    }
    // only ever goes up: a level that rises then drops again is not displayed twice
    else if (_blinksLeft == 0 and _candidateCount > _displayedCount)
    {
      _displayedCount = _candidateCount;
      _blinksLeft = _candidateCount;
      _lastBlinkTime_ms = time_ms - blinkPeriod_ms;
    }
  }

  if (_blinksLeft == 0 or not isLevelValid or not canBlink or time_ms - _lastBlinkTime_ms < blinkPeriod_ms)
    return false;

  _blinksLeft--;
  _lastBlinkTime_ms = time_ms;
  return true;
}

namespace {

BlinkTracker tracker;
bool isEnabled_s = true;

bool is_battery_level_valid()
{
  const auto& chargerState = component::charger::get_state();
  if (not chargerState.areMeasuresOk and not bsp::balancer::get_status().is_valid())
    return false;
  if (chargerState.is_effectivly_charging())
    return false;
  return not alerts::manager.is_raised(alerts::Type::BATTERY_MISSING) and
         not alerts::manager.is_raised(alerts::Type::BATTERY_READINGS_INCOHERENT);
}

} // namespace

void signal_output_on() { tracker.reset(hal::time_ms()); }

void loop()
{
  if (not isEnabled_s)
    return;

  // a blink during a user action could be mistaken for a feedback of this action
  const auto& buttonState = component::button::get_button_state();
  const bool canBlink =
          not component::outputPower::is_bliping() and not buttonState.isPressed and not buttonState.isLongPressed;

  if (tracker.update(hal::time_ms(), component::battery::get_battery_level(), is_battery_level_valid(), canBlink))
    user::handle_user_command(common::UserCommand::make_blip_output_command(blinkOffTime_ms));
}

void set_enabled(const bool isEnabled) { isEnabled_s = isEnabled; }

bool is_enabled() { return isEnabled_s; }

} // namespace battery_blink
} // namespace logic
} // namespace lampda

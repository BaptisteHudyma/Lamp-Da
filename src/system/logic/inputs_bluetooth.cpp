#include "inputs_bluetooth.h"

#include "src/user/functions.h"

#include "src/system/logic/behavior.h"
#include "src/system/logic/brightness_handle.h"
#include "src/system/logic/sunset_timer.h"

#include "src/system/platform/print.h"

#include "src/system/utils/utils.h"
#include <cstdint>

namespace lampda {
namespace logic {
namespace inputs_bluetooth {

/// keep track of the bluetooth uses
inline static bool _wasBluetoothUsed = false;

namespace __private {

/// handle the brightness command
void handle_brigthness_control(const uint8_t requiredBrigthness)
{
  // ignore if not on
  if (not logic::behavior::is_in_output_state())
    return;

  // update brightness
  static constexpr float brightnessMultiplier = ::lampda::brightness::absoluteMaximumBrightness / 100.0f;
  const brightness_t desiredBrightness =
          min<brightness_t>(::lampda::brightness::absoluteMaximumBrightness,
                            static_cast<brightness_t>(requiredBrigthness * brightnessMultiplier));
  // Update the system brightness for real, not the temporary. We want the changes to be saved
  logic::brightness::update_brightness(desiredBrightness);
  // update saved brightness
  logic::brightness::update_saved_brightness();
  // and change the sunset timer if needed
  logic::sunset::bump_timer();
}

/// handle the On or Off command
void handle_on_off_command(const bool shouldBeOn)
{
  // turn on
  if (shouldBeOn)
  {
    if (not logic::behavior::is_in_output_state())
      logic::behavior::set_power_on();
  }
  // turn off
  else
  {
    if (logic::behavior::is_in_output_state())
      logic::behavior::set_power_off();
  }
}

void handle_pattern_select_command(const uint8_t patternIndex, const uint32_t requestedColor = 0)
{
  // ignore if not on
  if (not logic::behavior::is_in_output_state())
    return;

  lampda::user::bluetooth_switch_pattern(patternIndex, requestedColor);
}

} // namespace __private

bool is_bluetooth_used() { return _wasBluetoothUsed; }

void handle_BLE_ELK_command(const utils::ELK::Package& elkControlCommand)
{
  _wasBluetoothUsed = true;

  switch (elkControlCommand.type)
  {
    case utils::ELK::Type::BRIGHTNESS:
      {
        __private::handle_brigthness_control(elkControlCommand.data[0]);
        break;
      }
    case utils::ELK::Type::ONOFF:
      {
        const bool shouldTurnOn = elkControlCommand.data[0] > 0;
        __private::handle_on_off_command(shouldTurnOn);
        break;
      }
    case utils::ELK::Type::COLOR_SELECT:
      {
        const uint32_t color =
                elkControlCommand.data[0] << 16 | elkControlCommand.data[1] << 8 | elkControlCommand.data[2];
        __private::handle_pattern_select_command(0, color);
        break;
      }
    case utils::ELK::Type::PATTERN_SELECT:
      {
        __private::handle_pattern_select_command(elkControlCommand.data[0] + 1);
        break;
      }
    // unhandled
    default:
      {
        platform::lampda_print("Unsupported ELK message type");
        break;
      }
  }
}

} // namespace inputs_bluetooth
} // namespace logic
} // namespace lampda

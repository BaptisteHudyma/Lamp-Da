#include "inputs_bluetooth.h"

#include "src/system/logic/behavior.h"
#include "src/system/logic/brightness_handle.h"
#include "src/system/logic/sunset_timer.h"

#include "src/system/platform/print.h"

#include "src/system/utils/utils.h"

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
    logic::behavior::set_power_on();
  }
  // turn off
  else
  {
    logic::behavior::set_power_off();
  }
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

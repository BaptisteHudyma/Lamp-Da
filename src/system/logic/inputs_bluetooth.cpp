#include "inputs_bluetooth.h"

#include "src/system/logic/behavior.h"
#include "src/system/logic/brightness_handle.h"
#include "src/system/logic/sunset_timer.h"

#include "src/system/platform/print.h"

#include "src/system/utils/utils.h"

namespace lampda {
namespace logic {
namespace inputs_bluetooth {

/**
 * \brief Handle an ELK bluetooth command
 */
void handle_BLE_ELK_command(const utils::ELK::Package& elkControlCommand)
{
  switch (elkControlCommand.type)
  {
    case utils::ELK::Type::BRIGHTNESS:
      {
        // update brightness
        static constexpr float brightnessMultiplier = ::lampda::brightness::absoluteMaximumBrightness / 100.0f;
        const brightness_t desiredBrightness =
                min<brightness_t>(::lampda::brightness::absoluteMaximumBrightness,
                                  static_cast<brightness_t>(elkControlCommand.data[0] * brightnessMultiplier));
        // Update the system brightness for real, not the temporary. We want the changes to be saved
        logic::brightness::update_brightness(desiredBrightness);
        // update saved brightness
        logic::brightness::update_saved_brightness();
        // and change the sunset timer if needed
        logic::sunset::bump_timer();
        break;
      }
    case utils::ELK::Type::ONOFF:
      {
        const bool shouldTurnOn = elkControlCommand.data[0] > 0;
        // turn on if not already on
        if (shouldTurnOn and not logic::behavior::is_in_output_state())
        {
          logic::behavior::set_power_on();
        }
        // turn off if not already off
        else if (not shouldTurnOn)
        {
          logic::behavior::set_power_off();
        }
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

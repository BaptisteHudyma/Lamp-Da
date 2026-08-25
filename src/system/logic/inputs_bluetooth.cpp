#include "inputs_bluetooth.h"

#include "src/user/functions.h"

#include "src/system/hal/bluetooth.h"

#include "src/system/bsp/text_out.h"

#include "src/system/logic/behavior.h"
#include "src/system/logic/brightness_handle.h"
#include "src/system/logic/sunset_timer.h"
#include "src/system/logic/user_commands.h"

#include "src/system/utils/elk_decoder.h"

#include "src/system/utils/utils.h"
#include <cstdint>

namespace lampda {
namespace logic {
namespace inputs_bluetooth {

/// keep track of the bluetooth uses
inline static bool _wasBluetoothUsed = false;

bool is_bluetooth_used() { return _wasBluetoothUsed || hal::bluetooth::was_used(); }

void handle_BLE_ELK_command(const utils::ELK::Package& elkControlCommand)
{
  _wasBluetoothUsed = true;

  switch (elkControlCommand.type)
  {
    case utils::ELK::Type::BRIGHTNESS:
      {
        static constexpr float brightnessMultiplier = ::lampda::brightness::absoluteMaximumBrightness / 100.0f;
        lampda::user::handle_user_command(UserCommand::make_brightness_command(
                static_cast<brightness_t>(elkControlCommand.data[0] * brightnessMultiplier)));
        return;
      }
    case utils::ELK::Type::ONOFF:
      {
        const bool shouldTurnOn = elkControlCommand.data[0] > 0;
        lampda::user::handle_user_command(UserCommand::make_turn_onoff_command(shouldTurnOn));
        return;
      }
    case utils::ELK::Type::PATTERN_SPEED:
      {
        const uint8_t speed = (elkControlCommand.data[0] / 100.0) * UINT8_MAX;
        lampda::user::handle_user_command(UserCommand::make_set_ramp_command(speed));
        return;
      }
    case utils::ELK::Type::SET_TIME:
      {
        const uint8_t hour = elkControlCommand.data[0];
        const uint8_t minutes = elkControlCommand.data[1];
        const uint8_t seconds = elkControlCommand.data[2];
        const uint8_t weekdayPlusOne = elkControlCommand.data[3];

        if (weekdayPlusOne <= 0 or weekdayPlusOne > 7)
        {
          bsp::lampda_print("Refused to set the time: invalid day of the week: %d", weekdayPlusOne);
          return;
        }
        component::time::RealTime time;
        time.hour = hour;
        time.minutes = minutes;
        time.seconds = seconds;
        time.dayOfTheWeek = weekdayPlusOne - 1;
        lampda::user::handle_user_command(UserCommand::make_set_real_time_command(time));
        return;
      }
    case utils::ELK::Type::TIMING:
      {
        // target hour, minute, seconds to turn on/off at
        const uint8_t hour = elkControlCommand.data[0];
        const uint8_t minutes = elkControlCommand.data[1];
        const uint8_t seconds = elkControlCommand.data[2];

        // auto turn on or off
        const bool isAutoTurnOn = elkControlCommand.data[3] == 0;
        // target days as a binary mask (unsuported)
        const uint8_t enabledDays = elkControlCommand.data[4];

        // mask the days to enable of the (enabledDays & daysMask)
        constexpr uint8_t daysMask = 127;
        // index bit of enable/disable
        constexpr uint8_t setClearBit = 1 << 7;

        const int indexOfTheDay = utils::get_index_of_first_set_bit(enabledDays);

        if (indexOfTheDay < 0 or indexOfTheDay > 6)
        {
          bsp::lampda_print("Refused to handle timing command: invalid day of the week");
          return;
        }

        component::time::RealTime time;
        time.hour = hour;
        time.minutes = minutes;
        time.seconds = seconds;
        time.dayOfTheWeek = indexOfTheDay;

        if (isAutoTurnOn)
        {
          bsp::lampda_print("Auto turn on mode is not implemented yet");
        }
        else
        {
          lampda::user::handle_user_command(UserCommand::make_set_sunset_to_time_command(time));
        }
        return;
      }
    case utils::ELK::Type::COLOR_SELECT:
      {
        lampda::user::handle_user_command(UserCommand::make_set_ble_custom_color_mode_command(
                elkControlCommand.data[0], elkControlCommand.data[1], elkControlCommand.data[2]));
        return;
      }
    case utils::ELK::Type::PATTERN_SELECT:
      {
        lampda::user::handle_user_command(UserCommand::make_set_ble_mode_command(elkControlCommand.data[0] + 1));
        return;
      }

    default:
      break;
  }
  //
  bsp::lampda_print("Unsupported ELK to User command convertion");
}

} // namespace inputs_bluetooth
} // namespace logic
} // namespace lampda

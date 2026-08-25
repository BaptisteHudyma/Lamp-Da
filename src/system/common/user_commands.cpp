#include "user_commands.h"

#include "src/system/utils/colorspace.h"

namespace lampda {
namespace common {

UserCommand::UserCommand() : _type(UserCommand::Type::Invalid), _dataCnt(0) {}

UserCommand UserCommand::make_set_ramp_command(const uint8_t ramp)
{
  UserCommand cmd;
  cmd._type = UserCommand::Type::SetUserRamp;
  cmd._dataCnt = 1;
  cmd._data[0] = ramp;
  return cmd;
}

UserCommand UserCommand::make_brightness_command(const brightness_t brightness)
{
  UserCommand cmd;
  cmd._type = UserCommand::Type::Brightness;
  cmd._dataCnt = 2;
  cmd._data[0] = (brightness & 0xFF);
  cmd._data[1] = (brightness >> 8) & 0xFF;
  return cmd;
}

UserCommand UserCommand::make_set_mode_command(const uint8_t groupId, const uint8_t modeId)
{
  UserCommand cmd;
  cmd._type = UserCommand::Type::SetMode;
  cmd._dataCnt = 2;
  cmd._data[0] = groupId;
  cmd._data[1] = modeId;
  return cmd;
}

UserCommand UserCommand::make_turn_onoff_command(const bool shouldTurnOn)
{
  UserCommand cmd;
  cmd._type = UserCommand::Type::OnOff;
  cmd._dataCnt = 1;
  cmd._data[0] = shouldTurnOn;
  return cmd;
}

UserCommand UserCommand::make_set_real_time_command(const component::time::RealTime& time)
{
  UserCommand cmd;
  cmd._type = UserCommand::Type::SetRealTime;
  cmd._dataCnt = 4;
  cmd._data[0] = time.hour;
  cmd._data[1] = time.minutes;
  cmd._data[2] = time.seconds;
  cmd._data[3] = time.dayOfTheWeek;
  return cmd;
}

UserCommand UserCommand::make_set_sunset_to_time_command(const component::time::RealTime& time)
{
  UserCommand cmd;
  cmd._type = UserCommand::Type::SetSunsetToTime;
  cmd._dataCnt = 4;
  cmd._data[0] = time.hour;
  cmd._data[1] = time.minutes;
  cmd._data[2] = time.seconds;
  cmd._data[3] = time.dayOfTheWeek;
  return cmd;
}

UserCommand UserCommand::make_set_ble_custom_color_mode_command(const uint8_t red,
                                                                const uint8_t green,
                                                                const uint8_t blue)
{
  UserCommand cmd;
  cmd._type = UserCommand::Type::SetBleCustomColorMode;
  cmd._dataCnt = 3;
  cmd._data[0] = red;
  cmd._data[1] = green;
  cmd._data[2] = blue;
  return cmd;
}

UserCommand UserCommand::make_set_ble_mode_command(const uint8_t index)
{
  UserCommand cmd;
  cmd._type = UserCommand::Type::SetBleMode;
  cmd._dataCnt = 1;
  cmd._data[0] = index;
  return cmd;
}

/**
 *
 *
 *
 */

bool UserCommand::parse_ramp(uint8_t& ramp) const
{
  if (_dataCnt != 1)
    return false;
  ramp = _data[0];
  return true;
}

bool UserCommand::parse_brightness(brightness_t& brightness) const
{
  if (get_type() != Type::Brightness or _dataCnt != 2)
    return false;
  const brightness_t tmpCrigthness = static_cast<brightness_t>((_data[1] << 8) | _data[0]);

  // check validity
  if (tmpCrigthness > ::lampda::brightness::absoluteMaximumBrightness)
    return false;

  brightness = tmpCrigthness;
  return true;
}

bool UserCommand::parse_set_mode(uint8_t& groupId, uint8_t& modeId) const
{
  if (get_type() != Type::SetMode or _dataCnt != 2)
    return false;
  groupId = _data[0];
  modeId = _data[1];
  return true;
}

bool UserCommand::parse_turn_onoff(bool& shouldTurnOn) const
{
  if (get_type() != Type::OnOff or _dataCnt != 1 or _data[0] > 1)
    return false;
  shouldTurnOn = _data[0];
  return true;
}

bool UserCommand::parse_set_real_time_command(component::time::RealTime& time) const
{
  if (get_type() != Type::SetRealTime or _dataCnt != 4)
    return false;
  component::time::RealTime tmpTime;
  tmpTime.hour = _data[0];
  tmpTime.minutes = _data[1];
  tmpTime.seconds = _data[2];
  tmpTime.dayOfTheWeek = _data[3];
  if (not tmpTime.is_valid())
    return false;

  time = tmpTime;
  return true;
}

bool UserCommand::parse_set_sunset_to_time_command(component::time::RealTime& time) const
{
  if (get_type() != Type::SetSunsetToTime or _dataCnt != 4)
    return false;
  component::time::RealTime tmpTime;
  tmpTime.hour = _data[0];
  tmpTime.minutes = _data[1];
  tmpTime.seconds = _data[2];
  tmpTime.dayOfTheWeek = _data[3];
  if (not tmpTime.is_valid())
    return false;

  time = tmpTime;
  return true;
}

bool UserCommand::parse_set_ble_custom_color_mode_command(uint32_t& color) const
{
  if (get_type() != Type::SetBleCustomColorMode or _dataCnt != 3)
    return false;

  color = utils::ColorSpace::RGB(_data[0], _data[1], _data[2]).get_rgb().color;
  return true;
}

bool UserCommand::parse_set_ble_mode_command(uint8_t& index) const
{
  if (get_type() != Type::SetBleMode or _dataCnt != 1)
    return false;
  index = _data[0];
  return true;
}

} // namespace common
} // namespace lampda

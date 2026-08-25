#include "user_commands.h"

namespace lampda {
namespace logic {

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

bool UserCommand::parse_ramp(uint8_t& ramp) const
{
  if (_dataCnt != 1)
    return false;
  ramp = _data[0];
  return true;
}

bool UserCommand::parse_brightness(brightness_t& brightness) const
{
  if (_dataCnt != 2)
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
  if (_dataCnt != 2)
    return false;
  groupId = _data[0];
  modeId = _data[1];
  return true;
}

} // namespace logic
} // namespace lampda

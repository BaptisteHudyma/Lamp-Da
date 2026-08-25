/*!
 * \file user_commands.h
 * This file contains the command wrapper to handle all commands from the system to the user processes.
 */

#pragma once

#include <array>
#include <cstdint>

#include "src/system/utils/constants.h"

namespace lampda {
namespace logic {

/// Define a command destined to the user layer.
/// Command can only be created by the static make_* methods.
class UserCommand
{
public:
  /// Command type
  enum class Type
  {
    Invalid,     ///< invalid placeholder message
    SetUserRamp, ///< set the user ramp value
    Brightness,  ///< set brightness
    SetMode,     ///< set mode index
  };
  static constexpr uint8_t maxDataSize = 8;

  Type get_type() const { return _type; }

  /**
   * \brief Set this commant to handle user ramp change
   * \param[in] ramp
   * \return The correct user command
   */
  static UserCommand make_set_ramp_command(const uint8_t ramp);

  /**
   * \brief Set this commant to handle brightness
   * \param[in] brightness
   * \return The correct user command
   */
  static UserCommand make_brightness_command(const brightness_t brightness);

  /**
   * \brief Set this commant to handle mode switch
   * \param[in] groupId
   * \param[in] modeId
   * \return The correct user command
   */
  static UserCommand make_set_mode_command(const uint8_t groupId, const uint8_t modeId);

  /**
   *
   * Parsing functions
   *
   */

  /**
   * \brief Parse the current command for a ramp value
   * \param[out] ramp
   * \return True if the current message is valid, and parameter is set
   */
  bool parse_ramp(uint8_t& ramp) const;

  /**
   * \brief Parse the current command for a brightness value
   * \param[out] brightness
   * \return True if the current message is valid, and parameter is set
   */
  bool parse_brightness(brightness_t& brightness) const;

  /**
   * \brief Parse the current command for a set_mode values
   * \param[out] groupId
   * \param[out] modeId
   * \return True if the current message is valid, and parameters are set
   */
  bool parse_set_mode(uint8_t& groupId, uint8_t& modeId) const;

protected:
  /// Protected constructor to prevent unallowed uses
  UserCommand();

private:
  Type _type;                             ///< request type
  uint8_t _dataCnt;                       ///< used data count
  std::array<uint8_t, maxDataSize> _data; ///< actual stored data
};

} // namespace logic
} // namespace lampda

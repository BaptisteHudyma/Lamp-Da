/*! \file elk_decoder.h
    \brief Decoder of the Bluetooth ELK messages
*/

#ifndef UTILS_ELK_DECODER
#define UTILS_ELK_DECODER

#include <array>
#include <cstdint>

namespace lampda {
namespace utils {
/// decode ELK control messages
namespace ELK {

enum class Type : uint8_t
{
  INVALID = 0,     ///< invalid value
  ONOFF,           ///< turn output onand off
  BRIGHTNESS,      ///< set a brightness
  PATTERN_SELECT,  ///< select a display pattern in a list
  PATTERN_SPEED,   ///< set the current pattern speed
  COLOR_SELECT,    ///< set output as a target color
  MIC_ONOFF,       ///< turn on/off the microphone
  MIC_MODE,        ///< set the pattern changes type with the microphone
  MIC_SENSITIVITY, ///< set microphone sensitivity
  LED_ORDER,       ///< set the new RGB led ordering
  SET_TIME,        ///< set the system time to a set time
  TIMING,          ///< set a desired on/off time
};

struct Package
{
  Type type = Type::INVALID;

  uint8_t dataSize = 0;
  std::array<uint8_t, 5> data;
};

/**
 * \brief Decode a given message, assuming it it a ELK control message
 */
inline bool decode_ELK_message(const uint8_t* msg, uint16_t len, Package& package)
{
  package.type = Type::INVALID;
  package.dataSize = 0;

  // basic check: ELK messages are 9 char long
  if (len != 9)
    return false;
  // Must start with 7E and end with EF
  if (msg[0] != 0x7E || msg[8] != 0xEF)
    return false;

  switch (msg[2])
  {
    case 0x01: // BRIGHTNESS [0-100]
      {
        if (msg[3] > 100)
          return false;

        package.type = Type::BRIGHTNESS;
        package.data[0] = msg[3]; // [0; 100]
        package.dataSize = 1;
        return true;
      }
    case 0x02: // PATTERN_SPEED [0-100]
      {
        if (msg[3] > 100)
          return false;

        package.type = Type::PATTERN_SPEED;
        package.data[0] = msg[3]; // [0; 100]
        package.dataSize = 1;
        return true;
      }
    case 0x03:
      {
        // PATTERN_SELECT [0-28]
        if (msg[4] == 0x03)
        {
          const uint8_t patternId = (msg[3] >= 128) ? (msg[3] - 128) : msg[3];
          if (patternId > 28)
            return false;

          package.type = Type::PATTERN_SELECT;
          package.data[0] = patternId; // [0; 28]
          package.dataSize = 1;
          return true;
        }
        // MIC_MODE: 0 = Classic, 1 = Soft, 2 = Dynamic, 3 = Disco.
        else if (msg[4] == 0x04)
        {
          const uint8_t micMode = (msg[3] >= 128) ? (msg[3] - 128) : msg[3];
          if (micMode > 3)
            return false;

          package.type = Type::MIC_MODE;
          package.data[0] = micMode; // [0; 3]
          package.dataSize = 1;
          return true;
        }
        break;
      }
    case 0x04: // ONOFF
      {
        if (msg[3] > 0x01)
          return false;

        package.type = Type::ONOFF;
        package.data[0] = msg[3];
        package.dataSize = 1;
        return true;
      }
    case 0x05: // COLOR_SELECT
      {
        if (msg[3] == 0x03)
        {
          package.type = Type::COLOR_SELECT;
          package.data[0] = msg[4];
          package.data[1] = msg[5];
          package.data[2] = msg[6];
          package.dataSize = 3;
          return true;
        }
        break;
      }
    case 0x06: // MIC_SENSITIVITY [0-100]
      {
        if (msg[3] > 100)
          return false;

        package.type = Type::MIC_SENSITIVITY;
        package.data[0] = msg[3]; // [0; 100]
        package.dataSize = 1;
        return true;
      }
    case 0x07: // MIC_ONOFF
      {
        if (msg[3] > 0x01)
          return false;

        package.type = Type::MIC_ONOFF;
        package.data[0] = msg[3];
        package.dataSize = 1;
        return true;
      }

    case 0x81: // LED_ORDER
      {
        if (msg[3] <= 0 or msg[3] > 3)
          return false;
        if (msg[4] <= 0 or msg[4] > 3)
          return false;
        if (msg[5] <= 0 or msg[5] > 3)
          return false;
        // should contain only one 1, one 2 and one 3
        if (msg[3] + msg[4] + msg[5] != 1 + 2 + 3)
          return false;
        /// TODO: finish this check

        package.type = Type::LED_ORDER;
        package.data[0] = msg[3];
        package.data[1] = msg[4];
        package.data[2] = msg[5];
        package.dataSize = 3;
        return true;
      }
    case 0x82: // TIMING
      {
        package.type = Type::TIMING;
        package.data[0] = msg[3];
        package.data[1] = msg[4];
        package.data[2] = msg[5];
        package.data[3] = msg[6];
        package.data[4] = msg[7];
        package.dataSize = 5;
        return true;
      }
    case 0x83: // SET_TIME
      {
        package.type = Type::SET_TIME;
        package.data[0] = msg[3];
        package.data[1] = msg[4];
        package.data[2] = msg[5];
        package.data[3] = msg[6];
        package.dataSize = 4;
        return true;
      }
    default:
      break;
  }
  return false;
}

} // namespace ELK
} // namespace utils
} // namespace lampda
#endif

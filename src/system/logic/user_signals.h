/*! \file user_signals.h
    \brief Output user action signals on the serial and bluetooth links.

    Each signal is a single line: '$' + name + ':' + values separated by ':' + "\r\n".
    The '$' prefix separates them from timestamped logs ("<time>> ...").
    Names are short to avoid saturating slow outputs (BLE).
    This format is a public interface for external apps: do not change existing signals, only add new ones.

    - "$brgt:<0-1024>"       brightness changed
    - "$mode:<group>:<mode>" active mode changed
    - "$ramp:<0-255>"        custom ramp changed
    - "$ind:<0-2>"           indicator level changed (see logic::indicator::set_brightness_level)
*/

#ifndef LOGIC_USER_SIGNALS_H
#define LOGIC_USER_SIGNALS_H

#include <cstdint>
#include <cstdio>
#include <cstddef>

#include "src/system/bsp/text_out.h"

namespace lampda {
namespace logic {
namespace user_signals {

/// Max signal lenght with the "\r\n" end, so that a signal is sent as one uart message (never interleaved with other
/// prints)
static constexpr size_t maxSignalLenght = 32;
/// Max formatted signal lenght, without the "\r\n" end and with the terminating null
static constexpr size_t maxFormatLenght = maxSignalLenght - 2;

/**
 * \brief Format a signal, without the line end
 * \param[out] buffer Output, of size maxFormatLenght
 * \param[in] name Signal name
 * \param[in] values Signal values
 * \param[in] valueCount Number of values
 * \return The formatted lenght, 0 if the signal does not fit
 */
inline size_t format(char* buffer, const char* name, const uint32_t* values, const size_t valueCount)
{
  int len = snprintf(buffer, maxFormatLenght, "$%s", name);
  for (size_t i = 0; i < valueCount and len > 0 and static_cast<size_t>(len) < maxFormatLenght; ++i)
    len += snprintf(buffer + len, maxFormatLenght - len, ":%lu", static_cast<unsigned long>(values[i]));

  // snprintf returns the wanted lenght, so truncation shows as len >= buffer size
  if (len <= 0 or static_cast<size_t>(len) >= maxFormatLenght)
  {
    buffer[0] = '\0';
    return 0;
  }
  return static_cast<size_t>(len);
}

/// Format and send a signal
inline void send(const char* name, const uint32_t* values, const size_t valueCount)
{
  char buffer[maxFormatLenght];
  if (format(buffer, name, values, valueCount) > 0)
    bsp::lampda_print_raw("%s\r\n", buffer);
}

inline void send_brightness(const uint32_t brightness) { send("brgt", &brightness, 1); }

inline void send_mode(const uint8_t groupId, const uint8_t modeId)
{
  const uint32_t values[] = {groupId, modeId};
  send("mode", values, 2);
}

inline void send_ramp(const uint32_t ramp) { send("ramp", &ramp, 1); }

inline void send_indicator(const uint32_t level) { send("ind", &level, 1); }

} // namespace user_signals
} // namespace logic
} // namespace lampda

#endif

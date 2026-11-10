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

    Rate limiting: each signal is sent at most once every signalIntervalMs. A change that arrives while the
    interval is running is not sent at once, only the latest value is kept and sent when the interval ends.

    Call user_signals::loop() regularly (more often than signalIntervalMs) so that pending values get sent.
*/

#ifndef LOGIC_USER_SIGNALS_H
#define LOGIC_USER_SIGNALS_H

#include <cstdint>
#include <cstdio>
#include <cstddef>

#include "src/system/hal/time.h"

#include "src/system/bsp/text_out.h"

namespace lampda {
namespace logic {
namespace user_signals {

/// Max signal lenght with the "\r\n" end, so that a signal is sent as one uart message (never interleaved with other
/// prints)
static constexpr size_t maxSignalLenght = 32;
/// Max formatted signal lenght, without the "\r\n" end and with the terminating null
static constexpr size_t maxFormatLenght = maxSignalLenght - 2;
/// Minimum time between two sends of the same signal
static constexpr uint32_t signalIntervalMs = 100;

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

/// Format and send a signal immediately (no rate limiting)
inline void send(const char* name, const uint32_t* values, const size_t valueCount)
{
  char buffer[maxFormatLenght];
  if (format(buffer, name, values, valueCount) > 0)
    bsp::lampda_print_raw("%s\r\n", buffer);
}

namespace detail {

static constexpr size_t maxValues = 2;

/// Rate limiting state of one signal
struct Slot
{
  constexpr Slot(const char* signalName, const uint8_t valueCount, const bool skipRepeats) :
    name(signalName),
    count(valueCount),
    dedupe(skipRepeats)
  {
  }

  const char* name;
  uint8_t count;
  bool dedupe;                                ///< Do not send a value equal to the latest sent one
  bool pending = false;                       ///< A value is waiting to be sent
  bool force = false;                         ///< The pending value must be sent even if equal to the latest sent one
  bool hasSent = false;                       ///< At least one value was sent
  uint32_t sent[maxValues] = {0, 0};          ///< Latest sent values
  uint32_t pendingValues[maxValues] = {0, 0}; ///< Latest requested values
  uint32_t sentTimeMs = 0;                    ///< Time of the latest send
};

enum SlotId : size_t
{
  slotBrightness = 0,
  slotMode,
  slotRamp,
  slotIndicator,
  slotCount
};

inline Slot& slot(const SlotId id)
{
  static Slot slots[slotCount] = {
          Slot("brgt", 1, true),
          Slot("mode", 2, false),
          Slot("ramp", 1, true),
          Slot("ind", 1, true),
  };
  return slots[id];
}

inline bool same_as_sent(const Slot& s)
{
  if (not s.hasSent)
    return false;
  for (size_t i = 0; i < s.count; ++i)
    if (s.sent[i] != s.pendingValues[i])
      return false;
  return true;
}

/// Send the pending value if there is one and the interval since the latest send is over
inline void flush(Slot& s, const uint32_t nowMs)
{
  if (not s.pending)
    return;

  // the latest value is what the receiver already has: nothing to send
  if (s.dedupe and not s.force and same_as_sent(s))
  {
    s.pending = false;
    return;
  }

  // unsigned subtraction is safe with counter wrap around
  if (s.hasSent and (nowMs - s.sentTimeMs) < signalIntervalMs)
    return;

  send(s.name, s.pendingValues, s.count);
  for (size_t i = 0; i < s.count; ++i)
    s.sent[i] = s.pendingValues[i];
  s.hasSent = true;
  s.sentTimeMs = nowMs;
  s.pending = false;
  s.force = false;
}

/// Record the latest value of a signal and send it now if the interval allows
inline void submit(const SlotId id, const uint32_t* values, const bool forceSend = false)
{
  Slot& s = slot(id);
  for (size_t i = 0; i < s.count; ++i)
    s.pendingValues[i] = values[i];
  s.pending = true;
  s.force = s.force or forceSend;
  flush(s, hal::time_ms());
}

} // namespace detail

/**
 * \brief Send the pending signals whose interval is over
 * Call it regularly, more often than signalIntervalMs.
 */
inline void loop()
{
  const uint32_t nowMs = hal::time_ms();
  for (size_t i = 0; i < detail::slotCount; ++i)
    detail::flush(detail::slot(static_cast<detail::SlotId>(i)), nowMs);
}

inline void send_brightness(const uint32_t brightness) { detail::submit(detail::slotBrightness, &brightness); }

inline void send_mode(const uint8_t groupId, const uint8_t modeId)
{
  const uint32_t values[] = {groupId, modeId};
  detail::submit(detail::slotMode, values);
}

inline void send_ramp(const uint32_t ramp, const bool forceSend = false)
{
  detail::submit(detail::slotRamp, &ramp, forceSend);
}

inline void send_indicator(const uint32_t level) { detail::submit(detail::slotIndicator, &level); }

} // namespace user_signals
} // namespace logic
} // namespace lampda

#endif

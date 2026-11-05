/*! \file battery_blink.h
    \brief Blink the main output when the battery level goes under a threshold.
*/

#ifndef LOGIC_BATTERY_BLINK_H
#define LOGIC_BATTERY_BLINK_H

#include <cstdint>

namespace lampda {
namespace logic {
/// Signal the battery level steps to the user, by blinking the output during the output mode
namespace battery_blink {

/// A level must be held for this time to be displayed, the voltage drops on high current use
static constexpr uint32_t levelFilterWindow_ms = 1000;
/// A level passed before this output use time is never displayed
static constexpr uint32_t minOutputUseTime_ms = 10000;
/// Time the output is off during a blink
static constexpr uint32_t blinkOffTime_ms = 50;
/// Time between the start of two successive blinks
static constexpr uint32_t blinkPeriod_ms = 150;
static_assert(blinkOffTime_ms < blinkPeriod_ms, "The output must turn back on between two blinks");

/**
 * \brief Return the number of blinks of a battery level
 * \param[in] level The battery level, in percent * 100
 * \return 1 under 75%, 2 under 50%, 3 under 25%, and 0 otherwise
 */
constexpr uint8_t get_blink_count(const uint16_t level)
{
  return level < 2500 ? 3 : level < 5000 ? 2 : level < 7500 ? 1 : 0;
}

/// Decide when to blink, independently of the hardware
class BlinkTracker
{
public:
  /// Call when the output mode is entered: all the levels under the current one can be displayed again
  void reset(const uint32_t time_ms);

  /**
   * \brief Call regularly during the output mode
   * \param[in] time_ms The current time
   * \param[in] level The battery level, in percent * 100
   * \param[in] isLevelValid If false, \p level is ignored
   * \param[in] canBlink If false, the next blink is delayed
   * \return true if a blink should start now
   */
  bool update(const uint32_t time_ms, const uint16_t level, const bool isLevelValid, const bool canBlink);

private:
  uint32_t _outputStartTime_ms = 0;
  uint32_t _candidateStartTime_ms = 0;
  uint32_t _lastBlinkTime_ms = 0;
  uint8_t _candidateCount = 0;     ///< blink count of the level being filtered
  uint8_t _displayedCount = 0;     ///< greatest blink count displayed since the output mode was entered
  uint8_t _blinksLeft = 0;         ///< blinks left to do in the current sequence
  bool _hasReferenceLevel = false; ///< true once the level at the start of the output mode is known
};

/// Call when the output mode is entered
void signal_output_on();
/// Call on each main loop of the output mode, before the user loop
void loop();

/// Enable or disable the blinks. Saved with the system parameters
void set_enabled(const bool isEnabled);
/// Return true if the blinks are enabled
bool is_enabled();

} // namespace battery_blink
} // namespace logic
} // namespace lampda

#endif

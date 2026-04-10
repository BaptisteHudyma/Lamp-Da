#include "brightness_handle.h"

#include "src/user/functions.h"

#include "src/system/platform/time.h"
#include "utils.h"

namespace brightness {

/**
 * \brief Store the brigthness characteristics
 */
struct BrightnessParameters
{
  /// Upper bound for the brightness
  /// Can be updated to limit the max birghtness, but should never ecxeed absoluteMaximumBrightness
  brightness_t MaxBrightnessLimit = brightness::absoluteMaximumBrightness;

  /// Hold the current level of brightness.
  /// This is the true reference value that is stored in memory at system shutdown
  brightness_t BRIGHTNESS = 200;
  /// Temporary output, that can differ from BRIGHTNESS. Not save between modes
  brightness_t savedBrightness = 200;

  /// Store when update_brightness() was last called, in milliseconds
  uint32_t lastBrightnessUpdate = 0;
};
BrightnessParameters __internal;

brightness_t get_brightness() { return __internal.BRIGHTNESS; }

brightness_t get_max_brightness() { return __internal.MaxBrightnessLimit; }
void set_max_brightness(const brightness_t brg)
{
  __internal.MaxBrightnessLimit = min<brightness_t>(brg, brightness::absoluteMaximumBrightness);
}

void update_saved_brightness() { __internal.savedBrightness = __internal.BRIGHTNESS; }
brightness_t get_saved_brightness() { return __internal.savedBrightness; }

void update_brightness(const brightness_t newBrightness, const bool isInitialRead)
{
  // set to the user max limit (limit can be changed by system)
  const brightness_t trueNewBrightness = min<brightness_t>(newBrightness, get_max_brightness());

  if (__internal.BRIGHTNESS != trueNewBrightness)
  {
    __internal.BRIGHTNESS = trueNewBrightness;

    // do not call user functions when reading parameters
    if (!isInitialRead)
    {
      user::brightness_update(trueNewBrightness);
    }
  }

  __internal.lastBrightnessUpdate = time_ms();
}

uint32_t when_last_update_brightness() { return __internal.lastBrightnessUpdate; }

} // namespace brightness

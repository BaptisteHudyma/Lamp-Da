#include "brightness_handle.h"

#include "src/user/functions.h"
#include "utils.h"

namespace brightness {

namespace __internal {
// temporary upper bound for the brightness
static brightness_t MaxBrightnessLimit = maxBrightness;

// hold the current level of brightness out of the raise/lower animation
brightness_t BRIGHTNESS = 200; // default start value
brightness_t previousBrightness = 200;
} // namespace __internal

brightness_t get_brightness() { return __internal::BRIGHTNESS; }

brightness_t get_max_brightness() { return __internal::MaxBrightnessLimit; }
void set_max_brightness(const brightness_t brg) { __internal::MaxBrightnessLimit = min(brg, maxBrightness); }

void update_previous_brightness() { __internal::previousBrightness = __internal::BRIGHTNESS; }
brightness_t get_previous_brightness() { return __internal::previousBrightness; }

void update_brightness(const brightness_t newBrightness, const bool isInitialRead)
{
  // set to the user max limit (limit can be changed by system)
  const brightness_t trueNewBrightness = min(newBrightness, get_max_brightness());

  if (__internal::BRIGHTNESS != trueNewBrightness)
  {
    __internal::BRIGHTNESS = trueNewBrightness;

    // do not call user functions when reading parameters
    if (!isInitialRead)
    {
      user::brightness_update(trueNewBrightness);
    }
  }
}

} // namespace brightness
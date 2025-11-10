#include "brightness_handle.h"

#include "src/user/functions.h"

#include "src/system/platform/time.h"
#include "utils.h"

namespace brightness {

struct BrightnessParameters
{
  // temporary upper bound for the brightness
  brightness_t MaxBrightnessLimit = maxBrightness;

  // hold the current level of brightness out of the raise/lower animation
  brightness_t BRIGHTNESS = 200; // default start value
  brightness_t savedBrightness = 200;

  // hold when update_brightness() was last called
  uint32_t lastBrightnessUpdate = 0;
};
BrightnessParameters __internal;

brightness_t get_brightness() { return __internal.BRIGHTNESS; }

brightness_t get_max_brightness() { return __internal.MaxBrightnessLimit; }
void set_max_brightness(const brightness_t brg)
{
  __internal.MaxBrightnessLimit = min<brightness_t>(brg, maxBrightness);
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

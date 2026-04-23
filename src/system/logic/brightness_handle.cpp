#include "brightness_handle.h"

#include "src/user/functions.h"

#include "src/system/platform/time.h"

#include "src/system/utils/utils.h"
#include <limits>

namespace lampda {
namespace logic {
namespace brightness {

/**
 * \brief Store the brigthness characteristics
 */
struct BrightnessParameters
{
  /// Upper bound for the brightness
  /// Can be updated to limit the max brightness, but should never ecxeed absoluteMaximumBrightness
  brightness_t MaxBrightnessLimit = ::lampda::brightness::absoluteMaximumBrightness;
  /// Upper bound of the brightness, limited to MaxBrightnessLimit. This only locks user uses.
  brightness_t MaxUserBrightnessLimit = ::lampda::brightness::absoluteMaximumBrightness;

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

brightness_t get_max_user_brightness() { return __internal.MaxUserBrightnessLimit; }

void set_max_brightness(const brightness_t brg)
{
  __internal.MaxBrightnessLimit = min<brightness_t>(brg, ::lampda::brightness::absoluteMaximumBrightness);
  // update saved user brightness
  set_max_user_brightness(__internal.MaxUserBrightnessLimit);
}

void set_max_user_brightness(const brightness_t brg)
{
  // limited to the max system brightness
  __internal.MaxUserBrightnessLimit = min<brightness_t>(brg, get_max_brightness());
}

void update_saved_brightness()
{
  __internal.savedBrightness = __internal.BRIGHTNESS;
  __internal.MaxUserBrightnessLimit = __internal.MaxBrightnessLimit;
}

brightness_t get_saved_brightness() { return __internal.savedBrightness; }

void update_brightness(const brightness_t newBrightness, const bool shouldCallUserBrightnessCallback)
{
  // set to the user max limit (limit can be changed by system)
  const brightness_t trueNewBrightness = min<brightness_t>(newBrightness, get_max_brightness());

  if (__internal.BRIGHTNESS != trueNewBrightness)
  {
    __internal.BRIGHTNESS = trueNewBrightness;

    // do not call user functions when reading parameters
    if (!shouldCallUserBrightnessCallback)
    {
      user::brightness_update(trueNewBrightness);
    }
  }

  __internal.lastBrightnessUpdate = ::lampda::platform::time_ms();
}

void force_brightness_user_callback()
{
  // special check that maximum brightness is not maximum of the type
  static_assert(::lampda::brightness::absoluteMaximumBrightness < std::numeric_limits<brightness_t>::max());

  // send an invalid command, to avoid updating the brightness
  user::brightness_update(::lampda::brightness::absoluteMaximumBrightness + 1);
}

uint32_t when_last_update_brightness() { return __internal.lastBrightnessUpdate; }

} // namespace brightness
} // namespace logic
} // namespace lampda

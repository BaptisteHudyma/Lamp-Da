#ifndef BRIGHTNESS_HANDLE_H
#define BRIGHTNESS_HANDLE_H

#include "constants.h"

namespace brightness {

// return the current brightness value (in range 0 - brightness::absoluteMaximumBrightness)
brightness_t get_brightness();

// return the saved brightness level
brightness_t get_saved_brightness();

// return the maximum allowed brightness
brightness_t get_max_brightness();

// set the new max brightness (limited to brightness::absoluteMaximumBrightness)
void set_max_brightness(const brightness_t brg);

// update the saved brightness value with the current brightness
void update_saved_brightness();

/**
 * \brief update the internal brightness values
 * \param[in] newBrightness the new brightness value
 * \param[in] isInitialRead first call of this function when starting
 *\
 */
void update_brightness(const brightness_t newBrightness, const bool isInitialRead = false);

/// \brief Get time offset in milliseconds when brightness was last updated
uint32_t when_last_update_brightness();

} // namespace brightness

#endif

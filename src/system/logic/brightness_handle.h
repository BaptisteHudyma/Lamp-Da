/*! \file brightness_handle.h
    \brief Handle the output LED strip brightness.
*/

#ifndef BRIGHTNESS_HANDLE_H
#define BRIGHTNESS_HANDLE_H

#include "src/system/utils/constants.h"

namespace lampda {
namespace logic {
/// Handle the LED output brightness
namespace brightness {

/// Return the current brightness value (in range 0 - brightness::absoluteMaximumBrightness)
brightness_t get_brightness();

/// Return the saved brightness level
brightness_t get_saved_brightness();

/// Return the maximum allowed brightness
brightness_t get_max_brightness();

/// Set the new max brightness
/// \param[in] brg New brightness, limited to brightness::absoluteMaximumBrightness
void set_max_brightness(const brightness_t brg);

/// Update the saved brightness value with the current brightness
void update_saved_brightness();

/**
 * \brief update the internal brightness values
 * \param[in] newBrightness the new brightness value
 * \param[in] isInitialRead first call of this function when starting
 */
void update_brightness(const brightness_t newBrightness, const bool isInitialRead = false);

/// \brief Get time in milliseconds when brightness was last updated
uint32_t when_last_update_brightness();

} // namespace brightness
} // namespace logic
} // namespace lampda

#endif

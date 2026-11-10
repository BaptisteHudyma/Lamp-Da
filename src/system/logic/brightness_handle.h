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

/// Return the current brightness value
// (in range ::lampda::brightness::absoluteMinimumBrightness - brightness::absoluteMaximumBrightness).
/// It's value depends on a lot of factors and can be cantrolled by any actors.
brightness_t get_brightness();

/// Return the saved brightness level.
/// This should be the prefered option in all computations.
brightness_t get_saved_brightness();

/// Return the saved brightness level, with the user brightness scale applied.
/// This is the level that the user modes should follow. See \ref set_user_brightness_scale
brightness_t get_scaled_saved_brightness();

/// Return the maximum allowed brightness
brightness_t get_max_brightness();

/// Return the maximum allowed brightness for the user modes
brightness_t get_max_user_brightness();

/// Set the new max brightness
/// \param[in] brg New brightness, limited to brightness::absoluteMaximumBrightness
void set_max_brightness(const brightness_t brg);

/**
 * \brief Scale the brightness available to the user modes, without changing the saved brightness.
 * The maximum user brightness and the saved brightness scale together, so a mode animating around the saved
 * brightness keeps its look at any scale.
 * The scale is reset to 1 by \ref update_saved_brightness
 * \param[in] scale between 0 and 1
 */
void set_user_brightness_scale(const float scale);

/// Update the saved brightness value with the current brightness
void update_saved_brightness();

/**
 * \brief update the internal brightness values
 * \param[in] newBrightness the new brightness value
 * \param[in] shouldSkipUserCallbacks True if this will skip the user callback call
 */
void update_brightness(const brightness_t newBrightness, const bool shouldSkipUserCallbacks = false);

/// force an update call to the user brightness callback
void force_brightness_user_callback();

/// \brief Get time in milliseconds when brightness was last updated
uint32_t when_last_update_brightness();

} // namespace brightness

/// Handle the indicator brightness
namespace indicator {

/**
 * \brief Set the brightness of the RGB indicator.
 * Indicator level can be
 * 0 : indicator and alerts displayed at full brightness
 * 1 : indicator and alerts displayed at low brightness
 * 2 : indicator and alerts displayed at low brightness, indicator off when charging
 * After a level update, the normal indicator stays visible for 5 seconds, even while charging at level 2.
 */
void set_brightness_level(const uint8_t level);
/**
 * \brief Get the brightness of the RGB indicator.
 * Indicator level can be
 * 0 : indicator and alerts displayed at full brightness
 * 1 : indicator and alerts displayed at low brightness
 * 2 : indicator and alerts displayed at low brightness, indicator off when charging
 * After a level update, the normal indicator stays visible for 5 seconds, even while charging at level 2.
 */
uint8_t get_brightness_level();

/// indicates if the indicator should be visible or not
bool should_indicator_be_visible();

/**
 * \brief Replace the battery level color of the indicator by a fixed color.
 * The indicator animations (charge, output, ...) are kept, only their color changes.
 * Alerts are not affected and always take precedence. This color is not saved.
 */
void set_color_override(const uint8_t red, const uint8_t green, const uint8_t blue);
/// Restore the default (battery level) color of the indicator
void clear_color_override();
/**
 * \brief Get the color that replaces the battery level color, if any.
 * \param[out] color The override color, as a 0xRRGGBB value. Untouched if there is no override.
 * \return true if an override color is set
 */
bool get_color_override(uint32_t& color);

} // namespace indicator

} // namespace logic
} // namespace lampda

#endif

/*! \file button_bindings.h
    \brief User configurable links between click counts and default behaviors.

    The click action (NC) and the hold action (NH) of a behavior always move together, so that a binding change cannot
    split a behavior between two click counts. Two behaviors can share a click count if they do not use the same
    action, like the quick navigation (click only) and the scroll (hold only).

    Click counts 1 and 2 (on/off, next mode, brightness) and 10 and above (shutdown, resets) cannot be configured.
    3 clicks is always the group navigation, so only the behaviors without a click action can use 3 clicks.
*/

#ifndef BUTTON_BINDINGS_H
#define BUTTON_BINDINGS_H

#include <cstddef>
#include <cstdint>

namespace lampda {
namespace logic {
/// Handle the links between click counts and the configurable default behaviors.
namespace button_bindings {

/// Default behaviors that can be linked to a click count
enum class Behavior : uint8_t
{
  Favorite,        ///< click: jump to the favorites. hold: add or remove a favorite
  QuickNavigation, ///< click: jump to the first mode
  Sunset,          ///< hold: start or extend the sunset timer
  CustomRamp,      ///< hold: configure the custom ramp of the mode
  Indicator,       ///< click: next indicator brightness. hold: cycle the indicator brightness
  Scroll,          ///< hold: scroll across modes and groups
  None,            ///< no behavior, keep this value last
};

/// Number of configurable behaviors
static constexpr size_t behaviorCount = static_cast<size_t>(Behavior::None);

/// Highest click count a behavior can be linked to
static constexpr uint8_t maxClicks = 9;

/// Result of a binding change
enum class SetResult : uint8_t
{
  Success,             ///< the binding was changed
  UnsupportedBehavior, ///< this lamp type does not have this behavior
  ReservedClicks,      ///< this click count is outside of [get_min_clicks(behavior), maxClicks]
  SwapNotPossible,     ///< a behavior on this click count cannot use the click count of this behavior
};

/// Return the behavior whose click action is linked to this click count, or Behavior::None
Behavior get_click_behavior(const uint8_t clicks);

/// Return the behavior whose hold action is linked to this click count, or Behavior::None
Behavior get_hold_behavior(const uint8_t clicks);

/// Return the click count linked to this behavior, or 0 if this lamp type does not have this behavior
uint8_t get_clicks(const Behavior behavior);

/// Return the lowest click count this behavior can be linked to
uint8_t get_min_clicks(const Behavior behavior);

/// Return true if this lamp type has this behavior
bool is_supported(const Behavior behavior);

/// Return the name of this behavior, as used by the command line
const char* get_name(const Behavior behavior);

/**
 * \brief Find a behavior from its command line name
 * \param[in] name The behavior name
 * \return The behavior, or Behavior::None if the name is unknown
 */
Behavior get_behavior_from_name(const char* name);

/**
 * \brief Link a behavior to a click count
 *
 * The behaviors using the same actions on this click count swap with this behavior, if the result stays coherent.
 *
 * \param[in] behavior The behavior to move, with both its click and hold actions
 * \param[in] clicks The new click count
 * \return Success if the binding changed, or else the reason why nothing changed
 */
SetResult set_binding(const Behavior behavior, const uint8_t clicks);

/// Restore the default links
void reset_to_defaults();

/// Load the links from the user parameters. Missing or incoherent values give the default links
void load_from_memory();

/// Store the links in the user parameters
void write_to_memory();

} // namespace button_bindings
} // namespace logic
} // namespace lampda

#endif

#ifndef MODE_TYPE_H
#define MODE_TYPE_H

/** \file
 *
 *  \brief Basic interface types to implement custom user modes
 **/

#include <cstdint>

/// Contains basic interface types to implement custom user modes
namespace modes {

/** \brief Parent object for all custom user modes
 *
 * Implement a custom user mode as follow:
 *
 * ```
 *  // user mode definition
 *  struct MyCustomMode : public modes::BasicMode {
 *    static void loop(auto& ctx) {
 *      ctx.lamp.setBrightness(200);
 *
 *      // ... other things using lamp
 *    }
 *  };
 * ```
 *
 * Once defined, you can enable the mode by adding it to indexable_functions.cpp :
 *
 * ```
 *   using ManagerTy = modes::ManagerFor<
 *     modes::GroupFor<ModeA,
 *                     ModeB,
 *                     ...>,
 *     modes::GroupFor<Mode1,
 *                     MyCustomMode, // mode available between Mode1 & Mode2
 *                     Mode2>
 *   >;
 * ```
 *
 * You can start from the \link ./src/modes/custom/my_custom_mode.hpp mode
 * template \endlink below where you will need to remove the unused callbacks:
 *
 * \include modes/custom/my_custom_mode.hpp
 *
 * Further examples of modes are available in `src/modes/default`
 *
 * \remark BasicMode and all derived user modes should never be constructed,
 * use custom StateTy to implement stateful modes
 */
struct BasicMode
{
  /// Mode custom static state, made available through context (optional)
  struct StateTy
  {
  };

  /** \brief Custom user mode loop function (default)
   *
   * Loop function each tick called whenever the mode is set as the currently
   * active mode by the user
   *
   * \param[in] ctx The current context, providing a interface to the local
   * state, the mode manager, as well as the hardware through its lamp object
   */
  static void loop(auto& ctx) { return; }

  /** \brief Custom callback when mode gains focus (optional)
   *
   * Reset function is called once whenever mode is picked as the active mode
   *
   * \param[in] ctx The current context
   */
  static void reset(auto& ctx) { return; }

  /// Toggles the use of custom BasicMode::brightness_update() callback
  static constexpr bool hasBrightCallback = false;

  /** \brief Custom callback when brightness changes (optional)
   *
   * Callback active only if BasicMode::hasBrightCallback is True
   *
   * \param[in] ctx The current context
   * \param[in] brightness The brightness value set by the system
   * \remark Use update_brightness() to change brightness in order for this
   * callback to be correctly handled at runtime
   */
  static void brightness_update(auto& ctx, brightness_t brightness) { return; }

  /** \brief Toggles the use of custom ramps & BasicMode::custom_ramp_update()
   *
   * \remark Without this set to True, manager will NOT save custom ramp value!
   */
  static constexpr bool hasCustomRamp = false;

  /** \brief Custom callback when system sets user ramp (optional)
   *
   * This can be used to make a mode configurable through the 3H click+hold
   * user action, which will cycle through the 0-255 values in a ~2s ramp
   *
   * \param[in] ctx The current context
   * \param[in] rampValue The custom value set by the user
   * \remark This behavior is in user::button_hold_default() and may be
   * prevented if custom "usermode UI" via custom_hold() is enabled
   */
  static void custom_ramp_update(auto& ctx, uint8_t rampValue) { return; }

  /// Toggles "usermode" button UI custom_click() and custom_hold()
  static constexpr bool hasButtonCustomUI = false;

  /** \brief Custom "usermode" button UI for "click" action (optional)
   *
   * Callback active only if BasicMode::hasButtonCustomUI is True
   *
   * \param[in] ctx The current context
   * \param[in] nbClick The number of clicks made by the user
   * \return Returns True if default UI action should be prevented
   */
  static bool custom_click(auto& ctx, uint8_t nbClick) { return false; }

  /** \brief Custom "usermode" button UI for "click+hold" action (optional)
   *
   * Callback active only if BasicMode::hasButtonCustomUI is True
   *
   * \param[in] ctx The current context
   * \param[in] nbClickAndHold The number of clicks made by the user
   * \param[in] isEndOfHoldEvent True if the user just released the button
   * \param[in] holdDuration The duration of the on-going hold event
   * \return Returns True if default action must be prevented
   * \remark When \p isEndOfHoldEvent is True, then \p holdDuration is zero
   */
  static bool custom_hold(auto& ctx, uint8_t nbClickAndHold, bool isEndOfHoldEvent, uint32_t holdDuration)
  {
    return false;
  }

  /** \brief Toggles advanced system callbacks, see \link hasSystemCallbacks
   * list here \endlink
   *
   * Required to use any of the following:
   *  - BasicMode::power_on_sequence(),
   *  - BasicMode::power_off_sequence()
   *  - BasicMode::read_parameters()
   *  - BasicMode::write_parameters()
   *
   * \remark By default, these callbacks are called for all modes (not only the
   * active one) and thus should be kept minimal
   */
  static constexpr bool hasSystemCallbacks = false;

  /** \brief Custom callback when the system powers on (optional)
   *
   * Callback active only if BasicMode::hasSystemCallbacks is True
   *
   * \param[in] ctx The current context
   * \remark This must be a non-blocking function
   */
  static void power_on_sequence(auto& ctx) { return; }

  /** \brief Custom callback when the system powers off (optional)
   *
   * Callback active only if BasicMode::hasSystemCallbacks is True
   *
   * \param[in] ctx The current context
   * \remark This must be a non-blocking function
   */
  static void power_off_sequence(auto& ctx) { return; }

  /** \brief Custom callback to read parameters from filesystem (optional)
   *
   * Callback active only if BasicMode::hasSystemCallbacks is True
   *
   * \param[in] ctx The current context
   */
  static void read_parameters(auto& ctx) { return; }

  /** \brief Custom callback to write parameters to filesystem (optional)
   *
   * Callback active only if BasicMode::hasSystemCallbacks is True
   *
   * \param[in] ctx The current context
   */
  static void write_parameters(auto& ctx) { return; }

  /** \brief Toggles the use of custom BasicMode::user_thread() callback
   *
   * \remark When this is enabled, default is to call hardware::LampTy's `show()`
   * in user::user_thread() after the BasicMode::user_thread() callback
   */
  static constexpr bool requireUserThread = false;

  /** \brief Custom secondary loop, executed in another thread (optional)
   *
   * Called only if BasicMode::requireUserThread is True
   *
   * \param[in] ctx The current context
   * \remark This is executed as prologue of user::user_thread() and hence
   * must complete quickly in order to keep the lamp responsive
   */
  static void user_thread(auto& ctx) { return; }

  /** \brief Store identifier for persistent storage (optional)
   *
   * By default, all modes are reset upon a shutdown, providing no persistence
   * of their state across several on/off cycles. To expose to the user a way
   * to configure your mode in a persistent fashion, you can use:
   *
   * ```
   *    static void loop(auto& ctx) {
   *      uint8_t rampValue = ctx.get_active_custom_ramp();
   *      // ... (use rampValue to pick a color or something) ...
   *    }
   *
   *    static constexpr bool hasCustomRamp = true; // required
   * ```
   *
   * This value is configurable through user interaction with the button. If
   * you need a persistent value, private to your mode, you can use:
   *
   * ```
   *    static void loop(auto& ctx) {
   *      uint8_t indexValue = ctx.get_active_custom_index();
   *      // ... (use indexValue to change how something works) ...
   *
   *      // at some point, indexValue is modified:
   *      indexValue += 1;
   *      ctx.set_active_custom_index(indexValue);
   *
   *      // next time mode is enabled, or saved as favorite, both custom
   *      // values (rampValue, indexValue) are remembered by default
   *    }
   *
   *    static constexpr bool hasCustomRamp = true; // also required
   * ```
   *
   * These are the two always-available persistent state that a mode can
   * configure by default. However, for more advanced modes (like games, with a
   * list of high-scores, for example) more flexibility can be required.
   *
   * If you need such extended storage, see ContextTy::KeyProxy for usage.
   *
   * If you don't need these capabilities, just ignore this identifier :)
   */
  static constexpr uint32_t storeId = 0;

  // modes shall not implement any constructors
  BasicMode() = delete;                            ///< \private
  BasicMode(const BasicMode&) = delete;            ///< \private
  BasicMode& operator=(const BasicMode&) = delete; ///< \private

  // polyfill (to be ignored in this context)
  static constexpr bool everyBrightCallback = false;    ///< \private
  static constexpr bool everySystemCallbacks = false;   ///< \private
  static constexpr bool everyRequireUserThread = false; ///< \private
  static constexpr bool everyCustomRamp = false;        ///< \private
  static constexpr bool everyButtonCustomUI = false;    ///< \private
};

} // namespace modes

#endif

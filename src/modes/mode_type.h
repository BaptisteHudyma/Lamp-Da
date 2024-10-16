#ifndef MODE_TYPE_H
#define MODE_TYPE_H

#include <cstdint>

/** \file mode_type.h
 *  \brief Basic interface types to implement custom user modes
 **/

/// Contains basic interface types to implement custom user modes
namespace modes {

/** \brief Parent object for all custom user modes
 *
 * Implement a custom user mode as follow:
 *
 * ```
 *  // simplified user mode definition
 *  struct MyCustomMode : public modes::BasicMode {
 *    static void loop(LedStrip& strip) {
 *      strip.clear();
 *
 *      // ... other things using strip
 *    }
 *  };
 * ```
 *
 * It is recommended to use the full user mode definition:
 *
 * ```
 *  // full user mode definition
 *  struct MyCustomMode : public modes::FullMode {
 *
 *   template <typename CtxTy>
 *   static void loop(CtxTy& ctx) {
 *      auto& strip = ctx.strip;
 *      strip.clear();
 *
 *      // ... other things using strip & ctx
 *   }
 *
 *  };
 * ```
 *
 * Once defined, you can enable the mode by adding it to user_functions.h :
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
 * You can start from the \link ./src/modes/custom/my_custom_mode.h mode
 * template \endlink below where you will need to remove the unused callbacks:
 *
 * \include modes/custom/my_custom_mode.h
 *
 * Further examples of modes are available in `src/modes/default`
 *
 * \remark BasicMode and all derived user modes should never be constructed,
 * use custom StateTy to implement stateful modes
 */
struct BasicMode {

  /// Mode custom static state, made available through context (optional)
  struct StateTy { };

  /** \brief Simplified user mode loop function (default)
   *
   * Loop function with a simplified prototype, which is called instead of full
   * loop(CtxTy&) if BasicMode::simpleMode is True
   *
   * \remark To make mode stateful, define custom StateTy and use loop(CtxTy&)
   * to retrieve context, then retrieve the state instance from context
   */
  static void loop(LedStrip& strip) { return; }

  /// Picks between simplified BasicMode::loop() and full BasicMode::loop(CtxTy&)
  static constexpr bool simpleMode = true;

  /** \brief Custom user mode loop function (optional)
   *
   * Loop function each tick called whenever the mode is set as the currently
   * active mode by the user
   *
   * \param[in] ctx The current context, providing a interface to the
   * controller and the LED strip, as well as an access to its state
   * \remark By default BasicMode::simpleMode is True and simplified loop() is
   * called instead of loop(CtxTy&)
   */
  template<typename CtxTy>
  static void loop(CtxTy& ctx) { return; }

  /** \brief Custom callback when mode gains focus (optional)
   *
   * Reset function is called once whenever mode is picked as the active mode
   *
   * \param[in] ctx The current context
   */
  template<typename CtxTy>
  static void reset(CtxTy& ctx) { return; }

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
  template<typename CtxTy>
  static void brightness_update(CtxTy& ctx, uint8_t brightness) { return; }

  /// Toggles the use BasicMode::custom_ramp_update() callback
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
  template<typename CtxTy>
  static void custom_ramp_update(CtxTy& ctx, uint8_t rampValue) {
    return;
  }

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
  template<typename CtxTy>
  static bool custom_click(CtxTy& ctx, uint8_t nbClick) {
    return false;
  }

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
  template<typename CtxTy>
  static bool custom_hold(CtxTy& ctx,
                          uint8_t nbClickAndHold,
                          bool isEndOfHoldEvent,
                          uint32_t holdDuration) {
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
  template<typename CtxTy>
  static void power_on_sequence(CtxTy& ctx) { return; }

  /** \brief Custom callback when the system powers off (optional)
   *
   * Callback active only if BasicMode::hasSystemCallbacks is True
   *
   * \param[in] ctx The current context
   * \remark This must be a non-blocking function
   */
  template<typename CtxTy>
  static void power_off_sequence(CtxTy& ctx) { return; }

  /** \brief Custom callback to read parameters from filesystem (optional)
   *
   * Callback active only if BasicMode::hasSystemCallbacks is True
   *
   * \param[in] ctx The current context
   */
  template<typename CtxTy>
  static void read_parameters(CtxTy& ctx) { return; }

  /** \brief Custom callback to write parameters to filesystem (optional)
   *
   * Callback active only if BasicMode::hasSystemCallbacks is True
   *
   * \param[in] ctx The current context
   */
  template<typename CtxTy>
  static void write_parameters(CtxTy& ctx) { return; }

  /** \brief Toggles the use of custom BasicMode::user_thread() callback
   *
   * \remark When this is enabled, default behavior is to only refresh strip in
   * user::user_thread() after the BasicMode::user_thread() callback
   */
  static constexpr bool requireUserThread = false;

  /** \brief Custom secondary loop, executed in another thread (optional)
   *
   * Called only if BasicMode::requireUserThread is True
   *
   * \param[in] ctx The current context
   * \remark This is executed as prologue of user::user_thread() and hence
   * must complete quickly in order to keep the strip responsive
   */
  template<typename CtxTy>
  static void user_thread(CtxTy& ctx) {
    return;
  }

  // modes shall not implement any constructors
  BasicMode() = delete; ///< \private
  BasicMode(const BasicMode&) = delete; ///< \private
  BasicMode& operator=(const BasicMode&) = delete; ///< \private
};

  /// Alias for BasicMode with BasicMode::simpleMode to False
  struct FullMode: public BasicMode {
    static constexpr bool simpleMode = false;
  };

} // namespace modes

#endif

#ifndef MODE_TYPES_H
#define MODE_TYPES_H

/** \file mode_types.h
 *  \brief Basic interface types to implement custom user modes
 **/

/// Contains basic interface types to implement custom user modes
namespace modes {

struct ModeTy {

  /// Mode custom static state, made available through context (optional)
  struct ModeStateTy { };

  /** \brief User mode loop function (default, mandatory)
   *
   * Loop function each tick called whenever the mode is set as the currently
   * active mode by the user
   *
   * \param[in] ctx The current context, providing a interface to the
   * controller and the LED strip, as well as an access to its state
   * \remark Prefer customizing ModeTy::ModeStateTy to add state to the mode,
   * such as a local persistent variable, instead of using static variables
   */
  template<typename CtxTy>
  static void loop(CtxTy& ctx) { return; }

  /** \brief Toggles the use of non-default additional optional callbacks
   *
   * Required to use any of the following:
   *  - ModeTy::brightness_update()
   *  - ModeTy::power_on_sequence(),
   *  - ModeTy::power_off_sequence()
   *  - ModeTy::read_parameters()
   *  - ModeTy::write_parameters()
   *
   */
  static constexpr bool hasExtraCallbacks = false;

  /** \brief Custom callback when brightness changes (optional)
   *
   * Callback active only if ModeTy::hasExtraCallbacks is True
   *
   * \param[in] ctx The current context
   * \param[in] brightness The brightness value set by the system
   * \remark Use update_brightness() to change brightness in order for this
   * callback to be correctly handled at runtime
   */
  template<typename CtxTy>
  static void brightness_update(CtxTy& ctx, uint8_t brightness) { return; }

  /** \brief Custom callback when the system powers on (optional)
   *
   * Callback active only if ModeTy::hasExtraCallbacks is True
   *
   * \param[in] ctx The current context
   * \remark This must be a non-blocking function!
   */
  template<typename CtxTy>
  static void power_on_sequence(CtxTy& ctx) { return; }

  /** \brief Custom callback when the system powers off (optional)
   *
   * Callback active only if ModeTy::hasExtraCallbacks is True
   *
   * \param[in] ctx The current context
   * \remark This must be a non-blocking function!
   */
  template<typename CtxTy>
  static void power_off_sequence(CtxTy& ctx) { return; }

  /** \brief Custom callback to write parameters to filesystem (optional)
   *
   * Callback active only if ModeTy::hasExtraCallbacks is True
   *
   * \param[in] ctx The current context
   */
  template<typename CtxTy>
  static void write_parameters(CtxTy& ctx) { return; }

  /** \brief Custom callback to read parameters from filesystem (optional)
   *
   * Callback active only if ModeTy::hasExtraCallbacks is True
   *
   * \param[in] ctx The current context
   */
  template<typename CtxTy>
  static void read_parameters(CtxTy& ctx) { return; }

  /** \brief Toggles the use of alternate "user_thread" loop
   *
   * Required to use ModeTy::user_thread()
   */
  static constexpr bool requireUserThread = false;

  /** \brief Custom secondary loop, executed in another thread (optional)
   *
   * Called only if ModeTy::requireUserThread is True
   *
   * \param[in] ctx The current context
   */
  template<typename CtxTy>
  static void user_thread(CtxTy& ctx) {
    return;
  }

  /** \brief Toggles the use of the custom ramp for the active mode
   *
   * Required to use ModeTy::custom_ramp_update()
   */
  static constexpr bool hasCustomRamp = false;

  /** \brief Custom callback when system sets user ramp (optional)
   *
   * This can be used to make a mode configurable through the 3H click+hold
   * user action, which will cycle through the 0-255 values in a ~2s ramp
   *
   * \param[in] ctx The current context
   * \param[in] rampValue The custom value set by the user
   * \remark This behavior uses user::button_hold_default() and may be
   * prevented if custom "usermode UI" via ModeTy::custom_hold() is enabled
   */
  template<typename CtxTy>
  static void custom_ramp_update(CtxTy& ctx, uint8_t rampValue) {
    return;
  }

  /** \brief Toggles the use of custom "usermode" button UI
   *
   * Required to use any of the following:
   *  - ModeTy::custom_click()
   *  - ModeTy::custom_hold()
   */
  static constexpr bool hasButtonCustomUI = false;

  /** \brief Custom "usermode" button UI for "click" action (optional)
   *
   * Callback active only if ModeTy::hasButtonCustomUI is True
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
   * Callback active only if ModeTy::hasButtonCustomUI is True
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
                          uint32_t holdDuration);
};

} // namespace modes

#endif

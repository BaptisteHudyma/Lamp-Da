#ifndef MY_CUSTOM_MODE_H
#define MY_CUSTOM_MODE_H

struct MyCustomMode : public modes::FullMode {

  template <typename CtxTy>
  static void loop(CtxTy& ctx) { }

  template<typename CtxTy>
  static void reset(CtxTy& ctx) { }

  // only if hasBrightCallback
  template <typename CtxTy>
  static void brightness_update(CtxTy& ctx, uint8_t brightness) { }

  // only if hasCustomRamp
  template <typename CtxTy>
  static void custom_ramp_update(CtxTy& ctx, uint8_t rampValue) { }

  // only if hasButtonCustomUI
  template <typename CtxTy>
  static bool custom_click(CtxTy& ctx, uint8_t nbClick) {
    return false;
  }

  template <typename CtxTy>
  static bool custom_hold(CtxTy& ctx,
                          uint8_t nbClickAndHold,
                          bool isEndOfHoldEvent,
                          uint32_t holdDuration) {
    return false;
  }

  // only if hasSystemCallbacks
  template <typename CtxTy>
  static void power_on_sequence(CtxTy& ctx) { }

  template <typename CtxTy>
  static void power_off_sequence(CtxTy& ctx) { }

  template <typename CtxTy>
  static void read_parameters(CtxTy& ctx) { }

  template <typename CtxTy>
  static void write_parameters(CtxTy& ctx) { }

  // only if requireUserThread
  template <typename CtxTy>
  static void user_thread(CtxTy& ctx) { }

  // keep only if customized
  struct StateTy { };

  // keep only the ones you need (= true)
  static constexpr bool hasBrightCallback = false;
  static constexpr bool hasCustomRamp = false;
  static constexpr bool hasButtonCustomUI = false;
  static constexpr bool hasSystemCallbacks = false;
  static constexpr bool requireUserThread = false;
}

#endif

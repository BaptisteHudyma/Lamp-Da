#ifndef MY_CUSTOM_MODE_H
#define MY_CUSTOM_MODE_H

struct MyCustomMode : public modes::BasicMode
{
  static void loop(auto& ctx) {}
  static void reset(auto& ctx) {}

  // only if hasBrightCallback
  static void brightness_update(auto& ctx, brightness_t brightness) {}

  // only if hasCustomRamp
  static void custom_ramp_update(auto& ctx, uint8_t rampValue) {}

  // only if hasButtonCustomUI
  static bool custom_click(auto& ctx, uint8_t nbClick) { return false; }

  static bool custom_hold(auto& ctx, uint8_t nbClickAndHold, bool isEndOfHoldEvent, uint32_t holdDuration)
  {
    return false;
  }

  // only if hasSystemCallbacks
  static void power_on_sequence(auto& ctx) {}
  static void power_off_sequence(auto& ctx) {}
  static void read_parameters(auto& ctx) {}
  static void write_parameters(auto& ctx) {}

  // only if requireUserThread
  static void user_thread(auto& ctx) {}

  // keep only if customized
  struct StateTy
  {
  };

  // keep only the ones you need (= true)
  static constexpr bool hasBrightCallback = false;
  static constexpr bool hasCustomRamp = false;
  static constexpr bool hasButtonCustomUI = false;
  static constexpr bool hasSystemCallbacks = false;
  static constexpr bool requireUserThread = false;
};

#endif

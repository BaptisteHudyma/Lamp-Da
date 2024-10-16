#ifndef MODE_GROUP_H
#define MODE_GROUP_H

#include <cstdint>
#include <utility>
#include <tuple>

/** \file group_type.h
 *  \brief modes::GroupFor and associated definitions
 **/

#include "tools.h"
#include "context_type.h"
#include "manager_type.h"
#include "mode_type.h"

namespace modes {

/// \private Implementation details of modes::GroupFor
template <typename AllModes>
struct GroupTy {

  // tuple helper
  using SelfTy = GroupTy<AllModes>;
  using AllModesTy = AllModes;
  static constexpr uint8_t nbModes{std::tuple_size_v<AllModesTy>};

  template <uint8_t Idx>
  using ModeAt = std::tuple_element_t<Idx, AllModesTy>;

  // required to support group-level context
  using HasAnyMode = details::anyOf<AllModesTy>;
  static constexpr bool simpleMode = false;
  static constexpr bool hasBrightCallback = HasAnyMode::hasBrightCallback;
  static constexpr bool hasSystemCallbacks = HasAnyMode::hasSystemCallbacks;
  static constexpr bool requireUserThread = HasAnyMode::requireUserThread;
  static constexpr bool hasCustomRamp = HasAnyMode::hasCustomRamp;
  static constexpr bool hasButtonCustomUI = HasAnyMode::hasButtonCustomUI;

  // constructors
  GroupTy() = delete;
  GroupTy(const GroupTy&) = delete;
  GroupTy& operator=(const GroupTy&) = delete;

  /// \private Dispatch active mode to callback
  template<typename CtxTy, typename CallBack>
  static void LMBD_INLINE dispatch_mode(CtxTy& ctx, CallBack&& cb) {
    uint8_t modeId = ctx.get_active_mode(nbModes);

    details::unroll<nbModes>([&](auto Idx) LMBD_INLINE {
      if (Idx == modeId) {
        cb(context_as<ModeAt<Idx>>(ctx));
      }
    });
  }

  /// \private Forward each mode to callback (if eligible)
  template<bool systemCallbacksOnly, typename CtxTy, typename CallBack>
  static void LMBD_INLINE foreach_mode(CtxTy& ctx, CallBack&& cb) {
    if constexpr (systemCallbacksOnly) {
      details::unroll<nbModes>([&](auto Idx) LMBD_INLINE {
        if /* TODO: constexpr */ (ModeAt<Idx>::hasSystemCallbacks) {
          cb(context_as<ModeAt<Idx>>(ctx));
        }
      });
    } else {
      details::unroll<nbModes>([&](auto Idx) LMBD_INLINE {
        cb(context_as<ModeAt<Idx>>(ctx));
      });
    }
  }

  //
  // navigation
  //

  static constexpr bool isGroupManager = false;
  static constexpr bool isModeManager = true;

  template <typename CtxTy>
  static void next_mode(CtxTy& ctx) {
    uint8_t modeId = ctx.get_active_mode(nbModes);
    ctx.set_active_mode(modeId + 1, nbModes);
    ctx.reset_mode();
  }

  template <typename CtxTy>
  static void reset_mode(CtxTy& ctx) {
    dispatch_mode(ctx, [](auto mode) { mode.reset(); });
  }

  //
  // all the callbacks
  //

  template<typename CtxTy>
  static void loop(CtxTy& ctx) {
    dispatch_mode(ctx, [](auto mode) { mode.loop(); });
  }

  template<typename CtxTy>
  static void brightness_update(CtxTy& ctx, uint8_t brightness) {
    dispatch_mode(ctx, [&](auto mode) {
      mode.brightness_update(brightness);
    });
  }

  template<typename CtxTy>
  static void custom_ramp_update(CtxTy& ctx, uint8_t rampValue) {
    dispatch_mode(ctx, [&](auto mode) {
      mode.custom_ramp_update(rampValue);
    });
  }

  template<typename CtxTy>
  static bool custom_click(CtxTy& ctx, uint8_t nbClick) {
    bool retVal = false;
    dispatch_mode(ctx, [&](auto mode) {
      retVal = mode.custom_click(nbClick);
    });
    return retVal;
  }

  template<typename CtxTy>
  static bool custom_hold(CtxTy& ctx,
                          uint8_t nbClickAndHold,
                          bool isEndOfHoldEvent,
                          uint32_t holdDuration) {
    bool retVal = false;
    dispatch_mode(ctx, [&](auto mode) {
      retVal = mode.custom_hold(nbClickAndHold,
                                isEndOfHoldEvent,
                                holdDuration);
    });
    return retVal;
  }

  template<typename CtxTy>
  static void power_on_sequence(CtxTy& ctx) {
    foreach_mode<true>(ctx, [](auto mode) { mode.power_on_sequence(); });
  }

  template<typename CtxTy>
  static void power_off_sequence(CtxTy& ctx) {
    foreach_mode<true>(ctx, [](auto mode) { mode.power_off_sequence(); });
  }

  template<typename CtxTy>
  static void write_parameters(CtxTy& ctx) {
    foreach_mode<true>(ctx, [](auto mode) { mode.write_parameters(); });
  }

  template<typename CtxTy>
  static void read_parameters(CtxTy& ctx) {
    foreach_mode<true>(ctx, [](auto mode) { mode.read_parameters(); });
  }

  template<typename CtxTy>
  static void user_thread(CtxTy& ctx) {
    dispatch_mode(ctx, [](auto mode) { mode.user_thread(); });
  }
};

/** \brief Group together many different modes::BasicMode
 *
 * Binds all methods of the provided list of \p Modes and works together
 * with modes::ModeManagerTy to dispatch events and switch modes
 */
template <typename... Modes>
using GroupFor = GroupTy<std::tuple<Modes...>>;

} // namespace modes

#endif

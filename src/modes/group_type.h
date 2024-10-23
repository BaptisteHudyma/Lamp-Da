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

/// \private Return true if a mode failed verification to prevent extra errors
template <typename AllModes>
static constexpr bool verifyGroup() {
    constexpr bool inheritsFromBasicMode = details::allOf<AllModes>::inheritsFromBasicMode;
    static_assert(inheritsFromBasicMode, "All modes must inherit from modes::BasicMode!");

    bool acc = false;
    acc |= !inheritsFromBasicMode;
    return acc ;
}

/// \private Implementation details of modes::GroupFor
template <typename AllModes, bool earlyFail = verifyGroup<AllModes>()>
struct GroupTy {

  // tuple helper
  using SelfTy = GroupTy<AllModes>;
  using AllModesTy = AllModes;
  using AllStatesTy = details::StateTyFrom<AllModes>;
  static constexpr uint8_t nbModes{std::tuple_size_v<AllModesTy>};

  template <uint8_t Idx>
  using ModeAtRaw = std::tuple_element_t<Idx, AllModesTy>;

  template <uint8_t Idx>
  using ModeAt = std::conditional_t<!earlyFail, ModeAtRaw<Idx>, BasicMode>;

  // required to support group-level context
  using HasAnyMode = details::anyOf<AllModesTy, earlyFail>;
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
  template<typename CallBack>
  static void LMBD_INLINE dispatch_mode(auto& ctx, CallBack&& cb) {
    uint8_t modeId = ctx.get_active_mode(nbModes);

    details::unroll<nbModes>([&](auto Idx) LMBD_INLINE {
      if (Idx == modeId) {
        cb(context_as<ModeAt<Idx>>(ctx));
      }
    });
  }

  /// \private Forward each mode to callback (if eligible)
  template<bool systemCallbacksOnly, typename CallBack>
  static void LMBD_INLINE foreach_mode(auto& ctx, CallBack&& cb) {
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
  // state
  //

  struct StateTy {
    AllStatesTy modeStates;
  };

  template <typename Mode>
  static auto* LMBD_INLINE getStateOf(auto& manager) {
    using StateTy = typename Mode::StateTy;

    StateTy* substate = nullptr;
    details::unroll<nbModes>([&](auto Idx) {
      using ModeHere = ModeAt<Idx>;
      constexpr bool isHere = std::is_same_v<ModeHere, Mode>;

      if constexpr (isHere) {
        auto state = manager.template getStateGroupOf<SelfTy>();
        if (state) {
          substate = &std::get<StateTy>(state->modeStates);
        }
      }
    });

    return substate;
  }

  //
  // navigation
  //

  static constexpr bool isGroupManager = false;
  static constexpr bool isModeManager = true;

  static void next_mode(auto& ctx) {
    uint8_t modeId = ctx.get_active_mode(nbModes);
    ctx.set_active_mode(modeId + 1, nbModes);
    ctx.reset_mode();
  }

  static void reset_mode(auto& ctx) {
    dispatch_mode(ctx, [](auto mode) { mode.reset(); });
  }

  //
  // all the callbacks
  //

  static void loop(auto& ctx) {
    dispatch_mode(ctx, [](auto mode) { mode.loop(); });
  }

  static void brightness_update(auto& ctx, uint8_t brightness) {
    dispatch_mode(ctx, [&](auto mode) {
      mode.brightness_update(brightness);
    });
  }

  static void custom_ramp_update(auto& ctx, uint8_t rampValue) {
    dispatch_mode(ctx, [&](auto mode) {
      mode.custom_ramp_update(rampValue);
    });
  }

  static bool custom_click(auto& ctx, uint8_t nbClick) {
    bool retVal = false;
    dispatch_mode(ctx, [&](auto mode) {
      retVal = mode.custom_click(nbClick);
    });
    return retVal;
  }

  static bool custom_hold(auto& ctx,
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

  static void power_on_sequence(auto& ctx) {
    foreach_mode<true>(ctx, [](auto mode) { mode.power_on_sequence(); });
  }

  static void power_off_sequence(auto& ctx) {
    foreach_mode<true>(ctx, [](auto mode) { mode.power_off_sequence(); });
  }

  static void write_parameters(auto& ctx) {
    foreach_mode<true>(ctx, [](auto mode) { mode.write_parameters(); });
  }

  static void read_parameters(auto& ctx) {
    foreach_mode<true>(ctx, [](auto mode) { mode.read_parameters(); });
  }

  static void user_thread(auto& ctx) {
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

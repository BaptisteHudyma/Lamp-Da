#ifndef MANAGER_TYPE_H
#define MANAGER_TYPE_H

#include <cstdint>
#include <cassert>
#include <utility>
#include <tuple>
#include <array>

/** \file manager_type.h
 *  \brief modes::ManagerFor and associated definitions
 **/

#include "src/modes/tools.hpp"
#include "src/modes/context_type.hpp"

namespace modes {

/// \private Active state is designated by a 64 integer
union ActiveIndexTy {
  struct {
    uint8_t groupIndex; // group id (as ordered in the manager)
    uint8_t modeIndex; // mode id (as ordered in its group)
    uint8_t rampIndex; // ramp value (as set by the user)
    uint8_t customIndex; // custom (as set by the mode)
  };

  uint64_t rawIndex;
};

/// \private Contains the logic to handle a fixed-timestep range
struct RampHandlerTy {
  static constexpr uint32_t startPeriod = 128;

  // \in stepSpeed how long to wait before incrementing (ms)
  // \in rampSaturates does the ramp saturates, or else wrap around?
  RampHandlerTy(uint32_t stepSpeed = 16, bool rampSaturates = false)
    : stepSpeed{stepSpeed}, rampSaturates{rampSaturates},
      lastTimeMeasured{1000}, isForward{true} { }

  void LMBD_INLINE update_ramp(uint8_t rampValue,
                               uint32_t holdTime,
                               auto callback) {

    // restart the rampage
    if (holdTime < lastTimeMeasured) {
      lastTimeMeasured = holdTime;

      if (holdTime < startPeriod) {

        // toggle forward / backward direction
        isForward = !isForward;
        if (rampSaturates) {
          if (rampValue == 0) isForward = true;
          if (rampValue == 255) isForward = false;
        }
      }
      return;
    }

    // count how many step we advanced
    uint32_t lastCounter = lastTimeMeasured / stepSpeed;
    uint32_t nextCounter = holdTime / stepSpeed;
    if (nextCounter <= lastCounter)
      return;

    // apply steps
    for (uint32_t I = 0; I < nextCounter - lastCounter; ++I) {

      // increment iff possible
      if (isForward && (!rampSaturates || rampValue < 255))
        rampValue += 1;

      // decrement iff possible
      if (!isForward && (!rampSaturates || rampValue > 0))
        rampValue -= 1;

      // forward value to callback
      callback(rampValue);
    }

    lastTimeMeasured = holdTime;
  }

  uint32_t stepSpeed;
  bool rampSaturates;
  uint32_t lastTimeMeasured;
  bool isForward;
};

/// \private Implementation details of modes::ManagerFor
template <typename AllGroups>
struct ModeManagerTy {

  // tuple helpers
  using SelfTy = ModeManagerTy<AllGroups>;
  using AllGroupsTy = AllGroups;
  using AllStatesTy = details::StateTyFrom<AllGroups>;
  static constexpr uint8_t nbGroups{std::tuple_size_v<AllGroupsTy>};

  template <uint8_t Idx>
  using GroupAt = std::tuple_element_t<Idx, AllGroupsTy>;

  // required to support manager-level context
  using HasAnyGroup = details::anyOf<AllGroupsTy>;
  static constexpr bool simpleMode = false;
  static constexpr bool hasBrightCallback = HasAnyGroup::hasBrightCallback;
  static constexpr bool hasSystemCallbacks = HasAnyGroup::hasSystemCallbacks;
  static constexpr bool requireUserThread = HasAnyGroup::requireUserThread;
  static constexpr bool hasCustomRamp = HasAnyGroup::hasCustomRamp;
  static constexpr bool hasButtonCustomUI = HasAnyGroup::hasButtonCustomUI;

  // constructors
  ModeManagerTy(LedStrip& strip)
    : activeIndex{{0, 0, 0, 0}}, strip{strip} { }

  ModeManagerTy() = delete;
  ModeManagerTy(const ModeManagerTy&) = delete;
  ModeManagerTy& operator=(const ModeManagerTy&) = delete;

  /// \private Get root context bound to manager
  auto get_context() {
    return ContextTy<SelfTy, SelfTy>(*this);
  }

  /// \private Dispatch active group to callback
  template<typename CallBack>
  static void LMBD_INLINE dispatch_group(auto& ctx, CallBack&& cb) {
    uint8_t groupId = ctx.get_active_group(nbGroups);

    details::unroll<nbGroups>([&](auto Idx) LMBD_INLINE {
      if (Idx == groupId) {
        cb(context_as<GroupAt<Idx>>(ctx));
      }
    });
  }

  /// \private Forward each group to callback (if eligible)
  template<bool systemCallbacksOnly, typename CallBack>
  static void LMBD_INLINE foreach_group(auto& ctx, CallBack&& cb) {
    if constexpr (systemCallbacksOnly) {
      details::unroll<nbGroups>([&](auto Idx) LMBD_INLINE {
        using GroupHere = GroupAt<Idx>;
        constexpr bool hasCallbacks = GroupHere::hasSystemCallbacks;

        if constexpr (hasCallbacks) {
          cb(context_as<GroupAt<Idx>>(ctx));
        }
      });
    } else {
      details::unroll<nbGroups>([&](auto Idx) LMBD_INLINE {
        cb(context_as<GroupAt<Idx>>(ctx));
      });
    }
  }

  //
  // state
  //

  struct StateTy {
    AllStatesTy groupStates;
    RampHandlerTy rampHandler = {16};
    RampHandlerTy scrollHandler = {512};
    ActiveIndexTy currentFavorite = {0, 0, 0, 0};
    std::array<uint8_t, nbGroups> lastModeMemory = {};

    // (executed before user reset_mode)
    static void LMBD_INLINE reset_config(auto& ctx) {
      auto& self = ctx.state;
      self.rampHandler.rampSaturates = false;
    }
  };

  template <typename Group>
  StateTyOf<Group>* LMBD_INLINE getStateGroupOf() {
    using StateTy = StateTyOf<Group>;
    using OptionalTy = std::optional<StateTy>;

    StateTy* substate = nullptr;
    details::unroll<nbGroups>([&](auto Idx) LMBD_INLINE {
      using GroupHere = GroupAt<Idx>;
      constexpr bool IsHere = std::is_same_v<GroupHere, Group>;

      if constexpr (IsHere) {
        OptionalTy& opt = std::get<OptionalTy>(state.groupStates);
        if (!opt.has_value()) {
          opt.emplace();
        }

        StateTy& stateHere = *opt;
        substate = &stateHere;
      }
    });
    assert(substate != nullptr && "this should not have compiled at all!");

    static_assert(details::ModeBelongsTo<Group, AllGroups>);
    return substate;
  }

  template <typename Mode>
  StateTyOf<Mode>& LMBD_INLINE getStateOf() {
    using TargetStateTy = StateTyOf<Mode>;

    // Mode is unknown / as no state, return placeholder
    if constexpr (std::is_same_v<TargetStateTy, NoState>) {
      return placeholder;

    // Mode is ManagerTy, return our own state
    } else if constexpr (std::is_same_v<TargetStateTy, StateTy>) {
      return state;

    // Mode is GroupTy, return the state of the group
    } else if constexpr (details::GroupBelongsTo<Mode, AllGroups>) {
      TargetStateTy* substate = getStateGroupOf<Mode>();

      if (substate == nullptr) {
        substate = (TargetStateTy*) &placeholder;
        assert(false && "this code should be unreachable, but is it?");
      }

      return *substate;
    } else {
      static_assert(details::ModeExists<Mode, AllGroups>);

      // Mode is somewhere in a group, search for it, return its state
      TargetStateTy* substate = nullptr;

      details::unroll<nbGroups>([&](auto Idx) LMBD_INLINE {
        using Group = GroupAt<Idx>;
        using AllModes = typename Group::AllModesTy;
        if constexpr (details::ModeBelongsTo<Mode, AllModes>) {
          substate = Group::template getStateOf<Mode>(*this);
        }
      });
      assert(substate != nullptr && "this should not have compiled at all!");

      if (substate == nullptr) {
        substate = (TargetStateTy*) &placeholder;
        assert(false && "this code should be unreachable, but is it?");
      }

      return *substate;
    }
  }

  //
  // navigation
  //

  static constexpr bool isGroupManager = true;
  static constexpr bool isModeManager = true;

  static void next_group(auto& ctx) {
    uint8_t groupIdBefore = ctx.get_active_group(nbGroups);

    ctx.state.lastModeMemory[groupIdBefore] = ctx.get_active_mode();
    ctx.set_active_group(groupIdBefore + 1, nbGroups);

    uint8_t groupIdAfter = ctx.get_active_group(nbGroups);
    ctx.set_active_mode(ctx.state.lastModeMemory[groupIdAfter]);

    ctx.reset_mode();
  }

  static void next_mode(auto& ctx) {
    ctx.state.reset_config(ctx);
    dispatch_group(ctx, [](auto group) { group.next_mode(); });
  }

  static void jump_to_favorite(auto& ctx) {
    ctx.modeManager.activeIndex = ctx.state.currentFavorite;
    ctx.reset_mode();
  }

  static void set_favorite_now(auto& ctx) {
    ctx.state.currentFavorite = ctx.modeManager.activeIndex;
  }

  static void reset_mode(auto& ctx) {
    ctx.state.reset_config(ctx);
    dispatch_group(ctx, [](auto group) { group.reset_mode(); });
  }

  static uint8_t get_modes_count(auto& ctx) {
    uint8_t value = 0;
    dispatch_group(ctx, [&](auto group) { value = decltype(group)::LocalModeTy::nbModes; });
    return value;
  }

  //
  // all the callbacks
  //

  static void loop(auto& ctx) {
    dispatch_group(ctx, [](auto group) { group.loop(); });
  }

  static void brightness_update(auto& ctx, uint8_t brightness) {
    dispatch_group(ctx, [&](auto group) {
      group.brightness_update(brightness);
    });
  }

  static void power_on_sequence(auto& ctx) {
    foreach_group<true>(ctx, [](auto group) { group.power_on_sequence(); });
  }

  static void power_off_sequence(auto& ctx) {
    foreach_group<true>(ctx, [](auto group) { group.power_off_sequence(); });
  }

  static void write_parameters(auto& ctx) {
    foreach_group<true>(ctx, [](auto group) { group.write_parameters(); });
  }

  static void read_parameters(auto& ctx) {
    foreach_group<true>(ctx, [](auto group) { group.read_parameters(); });
  }

  static void user_thread(auto& ctx) {
    dispatch_group(ctx, [](auto group) { group.user_thread(); });
  }

  static void custom_ramp_update(auto& ctx, uint8_t rampValue) {
    dispatch_group(ctx, [&](auto group) {
      group.custom_ramp_update(rampValue);
    });
  }

  static bool custom_click(auto& ctx, uint8_t nbClick) {
    bool retVal = false;
    dispatch_group(ctx, [&](auto group) {
      retVal = group.custom_click(nbClick);
    });
    return retVal;
  }

  static bool custom_hold(auto& ctx,
                          uint8_t nbClickAndHold,
                          bool isEndOfHoldEvent,
                          uint32_t holdDuration) {
    bool retVal = false;
    dispatch_group(ctx, [&](auto group) {
      retVal = group.custom_hold(nbClickAndHold,
                                 isEndOfHoldEvent,
                                 holdDuration);
    });
    return retVal;
  }

  //
  // members with direct access
  //

  ActiveIndexTy activeIndex;
  LedStrip& strip;

  //
  // private members
  //

private:
  NoState placeholder;
  StateTy state;
};

/** \brief Group together several mode groups defined through modes::GroupFor
 *
 * Binds all methods of the provided list of \p Groups and dispatch events
 * while managing all the modes::BasicMode::StateTy states & other behaviors
 *
 * \remarks All enabled modes shall be enumerated in modes::GroupFor listed
 * as inside the modes::ManagerFor modes::ModeManagerTy singleton
 */
template <typename... Groups>
using ManagerFor = ModeManagerTy<std::tuple<Groups...>>;

} // namespace modes

#endif

#ifndef MANAGER_TYPE_H
#define MANAGER_TYPE_H

#include <cstdint>
#include <cassert>
#include <utility>
#include <tuple>

/** \file manager_type.h
 *  \brief modes::ManagerFor and associated definitions
 **/

#include "tools.h"
#include "context_type.h"

namespace modes {

/// \private Active state is designated by a 64 integer
union activeIndexTy {
  struct {
    uint8_t groupIndex; // group id (as ordered in the manager)
    uint8_t modeIndex; // mode id (as ordered in its group)
    uint8_t rampIndex; // ramp value (as set by the user)
    uint8_t customIndex; // custom (as set by the mode)
  };

  uint64_t rawIndex;
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
  };

  template <typename Group>
  auto* LMBD_INLINE getStateGroupOf() {
    using StateTy = StateTyOf<Group>;

    StateTy* substate = nullptr;
    details::unroll<nbGroups>([&](auto Idx) {
      using GroupHere = GroupAt<Idx>;
      constexpr bool IsHere = std::is_same_v<GroupHere, Group>;

      if constexpr (IsHere) {
        substate = &(std::get<StateTy>(state.groupStates));
      }
    });
    assert(substate != nullptr && "this should be impossible :(");

    static_assert(details::ModeBelongsTo<Group, AllGroups>);
    return substate;
  }

  template <typename Mode>
  StateTyOf<Mode>& LMBD_INLINE getStateOf() {
    using TargetStateTy = StateTyOf<Mode>;

    if constexpr (std::is_same_v<TargetStateTy, NoState>) {
      return placeholder;
    } else if constexpr (std::is_same_v<TargetStateTy, StateTy>) {
      return state;
    } else if constexpr (details::GroupBelongsTo<Mode, AllGroups>) {
      TargetStateTy* substate = getStateGroupOf<Mode>();

      if (substate == nullptr) {
        substate = (TargetStateTy*) &placeholder;
        assert(false && "using state placeholder should not happen!");
      }

      return *substate;
    } else {
      TargetStateTy* substate = nullptr;

      details::unroll<nbGroups>([&](auto Idx) {
        using Group = GroupAt<Idx>;
        using AllModes = typename Group::AllModesTy;
        if constexpr (details::ModeBelongsTo<Mode, AllModes>) {
          substate = Group::template getStateOf<Mode>(*this);
        }
      });
      assert(substate != nullptr && "this should be impossible :(");

      if (substate == nullptr) {
        substate = (TargetStateTy*) &placeholder;
        assert(false && "using state placeholder should not happen!");
      }

      static_assert(details::ModeExists<Mode, AllGroups>);
      return *substate;
    }
  }

  //
  // navigation
  //

  static constexpr bool isGroupManager = true;
  static constexpr bool isModeManager = true;

  static void next_group(auto& ctx) {
    uint8_t groupId = ctx.get_active_group(nbGroups);
    ctx.set_active_group(groupId + 1, nbGroups);
    ctx.set_active_mode(0); // TODO: persist last picked mode
    ctx.reset_mode();

    ctx.modeManager.modeHasChanged = true;
    ctx.modeManager.groupHasChanged = true;
  }

  static void next_mode(auto& ctx) {
    dispatch_group(ctx, [](auto group) { group.next_mode(); });

    ctx.modeManager.modeHasChanged = true;
    ctx.modeManager.groupHasChanged = false;
  }

  static void reset_mode(auto& ctx) {
    dispatch_group(ctx, [](auto group) { group.reset_mode(); });
  }

  //
  // all the callbacks
  //

  static void loop(auto& ctx) {
    dispatch_group(ctx, [](auto group) { group.loop(); });
    ctx.modeManager.modeHasChanged = false;
    ctx.modeManager.groupHasChanged = false;
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

  activeIndexTy activeIndex;

  LedStrip& strip;
  bool modeHasChanged;
  bool groupHasChanged;

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

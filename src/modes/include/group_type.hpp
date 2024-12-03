#ifndef MODE_GROUP_H
#define MODE_GROUP_H

/** \file
 *
 *  \brief modes::GroupFor and associated definitions
 **/

#include <cstdint>
#include <cassert>
#include <utility>
#include <optional>
#include <tuple>
#include <array>

#include "src/modes/include/context_type.hpp"
#include "src/modes/include/manager_type.hpp"
#include "src/modes/include/mode_type.hpp"
#include "src/modes/include/tools.hpp"

namespace modes {

/// \private Return true if a mode failed verification to prevent extra errors
template<typename AllModes> static constexpr bool verifyGroup()
{
  constexpr bool inheritsFromBasicMode = details::allOf<AllModes>::inheritsFromBasicMode;
  static_assert(inheritsFromBasicMode, "All modes must inherit from modes::BasicMode!");

  constexpr bool stateDefaultConstructible = details::allOf<AllModes>::stateDefaultConstructible;
  static_assert(stateDefaultConstructible, "All states must be default constructible!");

  bool acc = false;
  acc |= !inheritsFromBasicMode;
  acc |= !stateDefaultConstructible;
  return acc;
}

/// \private Implementation details of modes::GroupFor
template<typename AllModes, bool earlyFail = verifyGroup<AllModes>()> struct GroupTy
{
  // tuple helper
  using SelfTy = GroupTy<AllModes>;
  using AllModesTy = AllModes;
  using AllStatesTy = details::StateTyFrom<AllModes>;
  static constexpr uint8_t nbModes {std::tuple_size_v<AllModesTy>};

  // last mode index must not collide with modes::store::noModeIndex
  static_assert(nbModes < 32, "Maximum of 31 modes has been exceeded.");

  template<uint8_t Idx> using ModeAtRaw = std::tuple_element_t<Idx, AllModesTy>;
  template<uint8_t Idx> using ModeAt = std::conditional_t<!earlyFail, ModeAtRaw<Idx>, BasicMode>;

  // required to support group-level context
  using HasAnyMode = details::anyOf<AllModesTy, earlyFail>;
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
  template<typename CallBack> static void LMBD_INLINE dispatch_mode(auto& ctx, CallBack&& cb)
  {
    uint8_t modeId = ctx.get_active_mode(nbModes);

    details::unroll<nbModes>([&](auto Idx) LMBD_INLINE {
      if (Idx == modeId)
      {
        cb(context_as<ModeAt<Idx>>(ctx));
      }
    });
  }

  /// \private Forward each mode to callback (if eligible)
  template<bool systemCallbacksOnly, typename CallBack> static void LMBD_INLINE foreach_mode(auto& ctx, CallBack&& cb)
  {
    if constexpr (systemCallbacksOnly)
    {
      details::unroll<nbModes>([&](auto Idx) LMBD_INLINE {
        if /* TODO: constexpr */ (ModeAt<Idx>::hasSystemCallbacks)
        {
          cb(context_as<ModeAt<Idx>>(ctx));
        }
      });
    }
    else
    {
      details::unroll<nbModes>([&](auto Idx) LMBD_INLINE {
        cb(context_as<ModeAt<Idx>>(ctx));
      });
    }
  }

  //
  // store
  //

  // persistent values
  enum class Store : uint16_t
  {
    rampMemory,
    indexMemory
  };

  static constexpr uint32_t storeId = modes::store::derivateStoreId<modes::store::hash("GroupTyStoreId"), AllModesTy>;

  //
  // state
  //

  struct StateTy
  {
    AllStatesTy modeStates;
    std::array<uint8_t, nbModes> customRampMemory;
    std::array<uint8_t, nbModes> customIndexMemory;

    void LMBD_INLINE save_ramps(auto& ctx, uint8_t modeId)
    {
      if constexpr (ctx.hasCustomRamp)
      {
        ctx.state.customRampMemory[modeId] = ctx.get_active_custom_ramp();
        ctx.state.customIndexMemory[modeId] = ctx.get_active_custom_index();
      }
    }

    void LMBD_INLINE load_ramps(auto& ctx, uint8_t modeId)
    {
      if constexpr (ctx.hasCustomRamp)
      {
        ctx.set_active_custom_ramp(ctx.state.customRampMemory[modeId]);
        ctx.set_active_custom_index(ctx.state.customIndexMemory[modeId]);
      }
    }
  };

  template<typename Mode> static auto* LMBD_INLINE getStateOf(auto& manager)
  {
    using StateTy = typename Mode::StateTy;
    using OptionalTy = std::optional<StateTy>;

    StateTy* substate = nullptr;
    details::unroll<nbModes>([&](auto Idx) LMBD_INLINE {
      using ModeHere = ModeAt<Idx>;
      constexpr bool isHere = std::is_same_v<ModeHere, Mode>;

      if constexpr (isHere)
      {
        auto* state = manager.template getStateGroupOf<SelfTy>();
        if (state)
        {
          OptionalTy& opt = std::get<OptionalTy>(state->modeStates);
          if (!opt.has_value())
          {
            opt.emplace(); // all StateTy must be default-contructible :)
          }

          StateTy& stateHere = *opt;
          substate = &stateHere;
        }
      }
    });

    assert(substate != nullptr && "this should not have happened!");
    return substate;
  }

  //
  // navigation
  //

  static constexpr bool isGroupManager = false;
  static constexpr bool isModeManager = true;

  static void next_mode(auto& ctx)
  {
    uint8_t modeIdBefore = ctx.get_active_mode(nbModes);
    if constexpr (ctx.hasCustomRamp)
    {
      ctx.state.save_ramps(ctx, modeIdBefore);
    }

    ctx.set_active_mode(modeIdBefore + 1, nbModes);

    // reset new mode we switched to
    ctx.reset_mode();
  }

  static void reset_mode(auto& ctx)
  {
    if constexpr (ctx.hasCustomRamp)
    {
      uint8_t modeIdAfter = ctx.get_active_mode(nbModes);
      ctx.state.load_ramps(ctx, modeIdAfter);
    }

    dispatch_mode(ctx, [](auto mode) {
      mode.reset();
    });
  }

  //
  // all the callbacks
  //

  static void loop(auto& ctx)
  {
    dispatch_mode(ctx, [](auto mode) {
      mode.loop();
    });
  }

  static void brightness_update(auto& ctx, uint8_t brightness)
  {
    dispatch_mode(ctx, [&](auto mode) {
      mode.brightness_update(brightness);
    });
  }

  static void custom_ramp_update(auto& ctx, uint8_t rampValue)
  {
    dispatch_mode(ctx, [&](auto mode) {
      mode.custom_ramp_update(rampValue);
    });
  }

  static bool custom_click(auto& ctx, uint8_t nbClick)
  {
    bool retVal = false;
    dispatch_mode(ctx, [&](auto mode) {
      retVal = mode.custom_click(nbClick);
    });
    return retVal;
  }

  static bool custom_hold(auto& ctx, uint8_t nbClickAndHold, bool isEndOfHoldEvent, uint32_t holdDuration)
  {
    bool retVal = false;
    dispatch_mode(ctx, [&](auto mode) {
      retVal = mode.custom_hold(nbClickAndHold, isEndOfHoldEvent, holdDuration);
    });
    return retVal;
  }

  static void power_on_sequence(auto& ctx)
  {
    foreach_mode<true>(ctx, [](auto mode) {
      mode.power_on_sequence();
    });
  }

  static void power_off_sequence(auto& ctx)
  {
    foreach_mode<true>(ctx, [](auto mode) {
      mode.power_off_sequence();
    });
  }

  static void write_parameters(auto& ctx)
  {
    foreach_mode<true>(ctx, [](auto mode) {
      mode.write_parameters();
    });
  }

  static void read_parameters(auto& ctx)
  {
    foreach_mode<true>(ctx, [](auto mode) {
      mode.read_parameters();
    });
  }

  static void user_thread(auto& ctx)
  {
    dispatch_mode(ctx, [](auto mode) {
      mode.user_thread();
    });
  }
};

/** \brief Group together many different modes::BasicMode
 *
 * Binds all methods of the provided list of \p Modes and works together
 * with modes::ModeManagerTy to dispatch events and switch modes
 */
template<typename... Modes> using GroupFor = GroupTy<std::tuple<Modes...>>;

} // namespace modes

#endif

#ifndef MANAGER_TYPE_H
#define MANAGER_TYPE_H

/** \file
 *
 *  \brief modes::ManagerFor and associated definitions
 **/

#include <cstdint>
#include <utility>
#include <tuple>
#include <array>

#include <src/system/assert.h>

#include "src/modes/include/tools.hpp"
#include "src/modes/include/context_type.hpp"
#include "src/modes/include/default_config.hpp"
#include "src/modes/include/hardware/keystore.hpp"
#include "src/modes/include/hardware/lamp_type.hpp"

#include "src/modes/include/anims/ramp_update.hpp"

namespace modes {

/// \private Active state is designated by a 32-bit integer
union ActiveIndexTy
{
  struct
  {
    uint8_t groupIndex;  // group id (as ordered in the manager)
    uint8_t modeIndex;   // mode id (as ordered in its group)
    uint8_t rampIndex;   // ramp value (as set by the user)
    uint8_t customIndex; // custom (as set by the mode)
  };

  uint32_t rawIndex;

  // static constructors
  //
  static auto from(const uint8_t* arr)
  {
    ActiveIndexTy index = {arr[0], arr[1], arr[2], arr[3]};
    return index;
  }
};

/// \private Contains the logic to handle a fixed-timestep range
template<typename Config> struct RampHandlerTy
{
  using ConfigTy = Config;
  static constexpr uint32_t startPeriod = Config::rampStartPeriodMs;

  // \in stepSpeed how long to wait before incrementing (ms)
  // \in rampSaturates does the ramp saturates, or else wrap around?
  RampHandlerTy(uint32_t stepSpeed, bool rampSaturates = Config::defaultRampSaturates) :
    stepSpeed {stepSpeed},
    rampSaturates {rampSaturates},
    lastTimeMeasured {1000},
    isForward {true},
    animEffect {Config::defaultCustomRampAnimEffect},
    animChoice {Config::defaultCustomRampAnimChoice}
  {
  }

  void LMBD_INLINE update_ramp(uint8_t rampValue, uint32_t holdTime, auto callback)
  {
    // restart the rampage
    if (holdTime < lastTimeMeasured)
    {
      lastTimeMeasured = holdTime;

      if (holdTime < startPeriod)
      {
        // toggle forward / backward direction
        isForward = !isForward;
        if (rampSaturates)
        {
          if (rampValue == 0)
            isForward = true;
          if (rampValue == 255)
            isForward = false;
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
    for (uint32_t I = 0; I < nextCounter - lastCounter; ++I)
    {
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
  bool animEffect;
  bool animChoice;
  uint32_t lastTimeMeasured;
  bool isForward;
};

/// \private Implementation details of modes::ManagerFor
template<typename Config, typename AllGroups> struct ModeManagerTy
{
  using SelfTy = ModeManagerTy<Config, AllGroups>;
  using ConfigTy = Config;
  using AllGroupsTy = AllGroups;
  using AllStatesTy = details::StateTyFrom<AllGroups>;
  static constexpr uint8_t nbGroups {std::tuple_size_v<AllGroupsTy>};

  // last group index must not collide with modes::store::noGroupIndex
  static_assert(nbGroups < 16, "Maximum of 15 groups has been exceeded.");

  template<uint8_t Idx> using GroupAt = std::tuple_element_t<Idx, AllGroupsTy>;

  // required to support manager-level context
  using HasAnyGroup = details::anyOf<AllGroupsTy>;
  static constexpr bool hasBrightCallback = HasAnyGroup::hasBrightCallback;
  static constexpr bool requireUserThread = HasAnyGroup::requireUserThread;
  static constexpr bool hasCustomRamp = HasAnyGroup::hasCustomRamp;
  static constexpr bool hasSystemCallbacks = hasCustomRamp || HasAnyGroup::hasSystemCallbacks;
  static constexpr bool hasButtonCustomUI = HasAnyGroup::hasButtonCustomUI;

  // constructors
  ModeManagerTy(hardware::LampTy& lamp) : activeIndex {ActiveIndexTy::from(Config::initialActiveIndex)}, lamp {lamp} {}

  ModeManagerTy() = delete;
  ModeManagerTy(const ModeManagerTy&) = delete;
  ModeManagerTy& operator=(const ModeManagerTy&) = delete;

  /// \private Get root context bound to manager
  auto get_context() { return ContextTy<SelfTy, SelfTy>(*this); }

  /// \private Dispatch active group to callback
  template<typename CallBack> static void LMBD_INLINE dispatch_group(auto& ctx, CallBack&& cb)
  {
    uint8_t groupId = ctx.get_active_group(nbGroups);

    details::unroll<nbGroups>([&](auto Idx) LMBD_INLINE {
      if (Idx == groupId)
      {
        cb(context_as<GroupAt<Idx>>(ctx));
      }
    });
  }

  /// \private Forward each group to callback (if eligible)
  template<bool systemCallbacksOnly, typename CallBack> static void LMBD_INLINE foreach_group(auto& ctx, CallBack&& cb)
  {
    if constexpr (systemCallbacksOnly)
    {
      details::unroll<nbGroups>([&](auto Idx) LMBD_INLINE {
        using GroupHere = GroupAt<Idx>;
        constexpr bool hasCallbacks = GroupHere::hasSystemCallbacks;

        if constexpr (hasCallbacks)
        {
          cb(context_as<GroupAt<Idx>>(ctx));
        }
      });
    }
    else
    {
      details::unroll<nbGroups>([&](auto Idx) LMBD_INLINE {
        cb(context_as<GroupAt<Idx>>(ctx));
      });
    }
  }

  //
  // store
  //

  // persistent values
  enum class Store : uint16_t
  {
    lastActive,
    modeMemory,
    favoriteMode0,
    favoriteMode1,
    favoriteMode2,
    favoriteMode3
  };

  static constexpr uint32_t storeId = modes::store::derivateStoreId<modes::store::hash("ManagerStoreId"), AllGroupsTy>;

  //
  // state
  //

  struct StateTy
  {
    // All group states, containing all modes individual states
    AllStatesTy groupStates;

    // When switching group, remember which mode was on last time we visited it
    std::array<uint8_t, nbGroups> lastModeMemory = {};

    // Which mode was selected as favorite
    ActiveIndexTy currentFavorite0 = ActiveIndexTy::from(Config::defaultFavorite);
    ActiveIndexTy currentFavorite1 = ActiveIndexTy::from(Config::defaultFavorite);
    ActiveIndexTy currentFavorite2 = ActiveIndexTy::from(Config::defaultFavorite);
    ActiveIndexTy currentFavorite3 = ActiveIndexTy::from(Config::defaultFavorite);

    // (variables for pending favorite state machine)
    uint8_t isFavoritePending = 0;
    uint8_t whichFavoritePending = 0;
    uint8_t lastFavoriteStep = 0;
    uint32_t lastFavoriteJump = 0;

    // Ramp handlers: custom ramp (or "color ramp") and mode scroll ramp
    RampHandlerTy<Config> rampHandler = {Config::defaultCustomRampStepSpeedMs};
    RampHandlerTy<Config> scrollHandler = {Config::scrollRampStepSpeedMs};

    bool clearStripOnModeChange = Config::defaultClearStripOnModeChange;

    // special effects
    uint8_t skipNextFrameEffect = 0; // should the next .loop() mode be skipped?

    // inside lamp.config
    //  - skipFirstLedsForEffect = 0; // should the loop skip some lower LEDs?
    //  - skipFirstLedsForAmount = 0; // how many pixels to shave from the top?

    // configuration-related actions done before mode reset
    static void LMBD_INLINE before_reset(auto& ctx)
    {
      auto& self = ctx.state;
      self.rampHandler.rampSaturates = Config::defaultRampSaturates;
      self.rampHandler.stepSpeed = Config::defaultCustomRampStepSpeedMs;
      self.clearStripOnModeChange = Config::defaultClearStripOnModeChange;
    }

    // configuration-related actions done after mode reset
    static void LMBD_INLINE after_reset(auto& ctx)
    {
      auto& self = ctx.state;
      if (self.clearStripOnModeChange)
      {
        ctx.lamp.clear();
      }
    }
  };

  template<typename Group> StateTyOf<Group>* LMBD_INLINE getStateGroupOf()
  {
    using StateTy = StateTyOf<Group>;
    using OptionalTy = std::optional<StateTy>;

    StateTy* substate = nullptr;
    details::unroll<nbGroups>([&](auto Idx) LMBD_INLINE {
      using GroupHere = GroupAt<Idx>;
      constexpr bool IsHere = std::is_same_v<GroupHere, Group>;

      if constexpr (IsHere)
      {
        OptionalTy& opt = std::get<OptionalTy>(state.groupStates);
        if (!opt.has_value())
        {
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

  template<typename Mode> StateTyOf<Mode>& LMBD_INLINE getStateOf()
  {
    using TargetStateTy = StateTyOf<Mode>;

    // Mode is unknown / as no state, return placeholder
    if constexpr (std::is_same_v<TargetStateTy, NoState>)
    {
      return placeholder;

      // Mode is ManagerTy, return our own state
    }
    else if constexpr (std::is_same_v<TargetStateTy, StateTy>)
    {
      return state;

      // Mode is GroupTy, return the state of the group
    }
    else if constexpr (details::GroupBelongsTo<Mode, AllGroups>)
    {
      TargetStateTy* substate = getStateGroupOf<Mode>();

      if (substate == nullptr)
      {
        substate = (TargetStateTy*)&placeholder;
        assert(false && "this code should be unreachable, but is it?");
      }

      return *substate;
    }
    else
    {
      static_assert(details::ModeExists<Mode, AllGroups>);

      // Mode is somewhere in a group, search for it, return its state
      TargetStateTy* substate = nullptr;

      details::unroll<nbGroups>([&](auto Idx) LMBD_INLINE {
        using Group = GroupAt<Idx>;
        using AllModes = typename Group::AllModesTy;
        if constexpr (details::ModeBelongsTo<Mode, AllModes>)
        {
          substate = Group::template getStateOf<Mode>(*this);
        }
      });
      assert(substate != nullptr && "this should not have compiled at all!");

      if (substate == nullptr)
      {
        substate = (TargetStateTy*)&placeholder;
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

  static void next_group(auto& ctx)
  {
    // change current group
    uint8_t groupIdBefore = ctx.get_active_group(nbGroups);
    ctx.set_active_group(groupIdBefore + 1, nbGroups);

    // restore last mode used in group, after switching
    uint8_t groupIdAfter = ctx.get_active_group(nbGroups);
    ctx.set_active_mode(ctx.state.lastModeMemory[groupIdAfter]);

    // reset new mode picked after group change
    ctx.reset_mode();
  }

  static void next_mode(auto& ctx)
  {
    ctx.state.before_reset(ctx);
    dispatch_group(ctx, [](auto group) {
      group.next_mode();
    });
    ctx.state.after_reset(ctx);
  }

  static void jump_to_favorite(auto& ctx, uint8_t which_one = 0)
  {
    auto keyActive = ctx.template storageFor<Store::lastActive>(ctx.modeManager.activeIndex);

    auto targetFavorite = ActiveIndexTy::from(Config::defaultFavorite);
    if (which_one == 0)
      targetFavorite = ctx.state.currentFavorite0;
    if (which_one == 1)
      targetFavorite = ctx.state.currentFavorite1;
    if (which_one == 2)
      targetFavorite = ctx.state.currentFavorite2;
    if (which_one == 3)
      targetFavorite = ctx.state.currentFavorite3;

    // reset once with the right mode
    ctx.modeManager.activeIndex = targetFavorite;
    ctx.reset_mode();

    // if ramps loaded in reset_mode != ones saved as favorite, emulate update
    if (ctx.modeManager.activeIndex.rawIndex != targetFavorite.rawIndex)
    {
      ctx.modeManager.activeIndex = targetFavorite;
      custom_ramp_update(ctx, ctx.get_active_custom_ramp());
    }
  }

  static void set_favorite_now(auto& ctx, uint8_t which_one = 0)
  {
    if (which_one == 0)
    {
      auto keyFav = ctx.template storageFor<Store::favoriteMode0>(ctx.state.currentFavorite0);
      ctx.state.currentFavorite0 = ctx.modeManager.activeIndex;
    }
    else if (which_one == 1)
    {
      auto keyFav = ctx.template storageFor<Store::favoriteMode1>(ctx.state.currentFavorite1);
      ctx.state.currentFavorite1 = ctx.modeManager.activeIndex;
    }
    else if (which_one == 2)
    {
      auto keyFav = ctx.template storageFor<Store::favoriteMode2>(ctx.state.currentFavorite2);
      ctx.state.currentFavorite2 = ctx.modeManager.activeIndex;
    }
    else if (which_one == 3)
    {
      auto keyFav = ctx.template storageFor<Store::favoriteMode3>(ctx.state.currentFavorite3);
      ctx.state.currentFavorite3 = ctx.modeManager.activeIndex;
    }
  }

  static void reset_mode(auto& ctx)
  {
    ctx.state.before_reset(ctx);
    dispatch_group(ctx, [](auto group) {
      group.reset_mode();
    });
    ctx.state.after_reset(ctx);
  }

  static uint8_t get_modes_count(auto& ctx)
  {
    uint8_t value = 0;
    dispatch_group(ctx, [&](auto group) {
      value = decltype(group)::LocalModeTy::nbModes;
    });
    return value;
  }

  static void quit_group(auto& ctx)
  {
    //
    uint8_t modeIdBefore = ctx.get_active_mode();

    // changes to lastModeMemory made in this function will be persistent
    auto keyModeMemory = ctx.template storageFor<Store::modeMemory>(ctx.state.lastModeMemory);

    // save last mode used in group, before switching
    uint8_t groupIdBefore = ctx.get_active_group(nbGroups);
    ctx.state.lastModeMemory[groupIdBefore] = modeIdBefore;
  }

  static void quit_mode(auto& ctx)
  {
    // TODO: this quit_mode is redundant with the one in group_type, cannot switch mode without switching mode

    // give an occasion to the current group to save its custom ramp
    uint8_t modeIdBefore = ctx.get_active_mode();
    dispatch_group(ctx, [&](auto group) {
      if constexpr (group.hasCustomRamp)
      {
        group.state.save_ramps(group, modeIdBefore);
      }
    });
  }

  //
  // all the callbacks
  //

  static void loop(auto& ctx)
  {
    if (ctx.state.isFavoritePending > 0)
    {
      ctx.state.isFavoritePending -= 1;

      if (ctx.state.isFavoritePending == 0 && ctx.state.whichFavoritePending < 4)
      {
        ctx.set_favorite_now(ctx.state.whichFavoritePending);
        alerts::manager.raise(alerts::Type::FAVORITE_SET);
      }
    }

    if (ctx.lamp.config.skipFirstLedsForEffect > 0)
    {
      ctx.lamp.config.skipFirstLedsForEffect -= 1;
    }

    if (ctx.state.skipNextFrameEffect > 0)
    {
      ctx.state.skipNextFrameEffect -= 1;
      return;
    }

    ctx.lamp.refresh_tick_value();

    dispatch_group(ctx, [](auto group) {
      group.loop();
    });
  }

  static void brightness_update(auto& ctx, brightness_t brightness)
  {
    dispatch_group(ctx, [&](auto group) {
      group.brightness_update(brightness);
    });
  }

  static void power_on_sequence(auto& ctx)
  {
    foreach_group<true>(ctx, [](auto group) {
      group.power_on_sequence();
    });
  }

  static void power_off_sequence(auto& ctx)
  {
    foreach_group<true>(ctx, [](auto group) {
      group.power_off_sequence();
    });
  }

  static void write_parameters(auto& ctx)
  {
    // this scope is the only one where parameters will be kept
    ctx.template storageSaveOnly<Store::lastActive>(ctx.modeManager.activeIndex);

    // save the 4 possible favorites
    ctx.template storageSaveOnly<Store::favoriteMode0>(ctx.modeManager.state.currentFavorite0);
    ctx.template storageSaveOnly<Store::favoriteMode1>(ctx.modeManager.state.currentFavorite1);
    ctx.template storageSaveOnly<Store::favoriteMode2>(ctx.modeManager.state.currentFavorite2);
    ctx.template storageSaveOnly<Store::favoriteMode3>(ctx.modeManager.state.currentFavorite3);

    foreach_group<not hasCustomRamp>(ctx, [](auto group) {
      if constexpr (group.hasCustomRamp)
      {
        group.state.save_ramps(group, group.get_active_mode());

        using StoreHere = typename decltype(group)::StoreEnum;
        group.template storageSaveOnly<StoreHere::rampMemory>(group.state.customRampMemory);
        group.template storageSaveOnly<StoreHere::indexMemory>(group.state.customIndexMemory);
      }

      group.write_parameters();
    });
  }

  static void read_parameters(auto& ctx)
  {
    // remove old filesystem data if we detect obsolete "storeId" serial
    using LocalStore = details::LocalStoreOf<decltype(ctx)>;
    LocalStore::template migrateStoreIfNeeded<storeId>();

    // load last active mode
    ctx.template storageLoadOnly<Store::lastActive>(ctx.modeManager.activeIndex);

    // load the 4 possible favorites
    ctx.template storageLoadOnly<Store::favoriteMode0>(ctx.state.currentFavorite0);
    ctx.template storageLoadOnly<Store::favoriteMode1>(ctx.state.currentFavorite1);
    ctx.template storageLoadOnly<Store::favoriteMode2>(ctx.state.currentFavorite2);
    ctx.template storageLoadOnly<Store::favoriteMode3>(ctx.state.currentFavorite3);

    // for each group, migrate & handle custom ramp memory
    foreach_group<not hasCustomRamp>(ctx, [](auto group) {
      using LocalStore = details::LocalStoreOf<decltype(group)>;
      LocalStore::template migrateStoreIfNeeded<storeId>();

      if constexpr (group.hasCustomRamp)
      {
        using StoreHere = typename LocalStore::EnumTy;
        group.template storageLoadOnly<StoreHere::rampMemory>(group.state.customRampMemory);
        group.template storageLoadOnly<StoreHere::indexMemory>(group.state.customIndexMemory);
      }

      group.read_parameters();
    });
  }

  static void user_thread(auto& ctx)
  {
    dispatch_group(ctx, [](auto group) {
      group.user_thread();
    });
  }

  static void custom_ramp_update(auto& ctx, uint8_t rampValue)
  {
    if (ctx.state.rampHandler.animEffect)
    {
      modes::anims::_rampAnimDispatch(ctx.state.rampHandler.animChoice, ctx, rampValue);
    }

    dispatch_group(ctx, [&](auto group) {
      group.custom_ramp_update(rampValue);
    });
  }

  static bool custom_click(auto& ctx, uint8_t nbClick)
  {
    bool retVal = false;
    dispatch_group(ctx, [&](auto group) {
      retVal = group.custom_click(nbClick);
    });
    return retVal;
  }

  static bool custom_hold(auto& ctx, uint8_t nbClickAndHold, bool isEndOfHoldEvent, uint32_t holdDuration)
  {
    bool retVal = false;
    dispatch_group(ctx, [&](auto group) {
      retVal = group.custom_hold(nbClickAndHold, isEndOfHoldEvent, holdDuration);
    });
    return retVal;
  }

  //
  // members with direct access
  //

  ActiveIndexTy activeIndex;
  hardware::LampTy& lamp;

  //
  // private members
  //

private:
  NoState placeholder;
  StateTy state;
};

/** \brief Same as modes::ManagerFor but with custom defaults
 *
 * See modes::DefaultManagerConfig for guide on how to override configuration.
 */
template<typename ManagerConfig, typename... Groups> using ManagerForConfig =
        ModeManagerTy<ManagerConfig, std::tuple<Groups...>>;

/** \brief Group together several mode groups defined through modes::GroupFor
 *
 * Binds all methods of the provided list of \p Groups and dispatch events
 * while managing all the modes::BasicMode::StateTy states & other behaviors
 *
 * \remarks All enabled modes shall be enumerated in modes::GroupFor listed
 * as inside the modes::ManagerFor modes::ModeManagerTy singleton
 */
template<typename... Groups> using ManagerFor = ModeManagerTy<DefaultManagerConfig, std::tuple<Groups...>>;

} // namespace modes

namespace modes::details {

//
// \private API
//

/// \private animate favorite picks
template<bool displayFavoriteNumber = true> void _animate_favorite_pick(auto& ctx, float holdDuration, float stepSize)
{
  // where we are: 0-255 rampColorRing
  uint32_t stepProgress = floor((holdDuration * 256.0) / stepSize);
  stepProgress = stepProgress % 256;

  // up to 5 step state: "which_one" is [0, 1, 2, 3] and "do not set" is 4
  uint32_t stepCount = 4 + floor(holdDuration / stepSize);
  stepCount = stepCount % 5;

  // display ramp to show where user is standing
  if (stepCount == 0)
    anims::rampColorRing(ctx, stepProgress, colors::PaletteGradient<colors::Green, colors::White>);
  if (stepCount == 1)
    anims::rampColorRing(ctx, stepProgress, colors::PaletteGradient<colors::Blue, colors::White>);
  if (stepCount == 2)
    anims::rampColorRing(ctx, stepProgress, colors::PaletteGradient<colors::Orange, colors::White>);
  if (stepCount == 3)
    anims::rampColorRing(ctx, stepProgress, colors::PaletteGradient<colors::Purple, colors::White>);
  if (stepCount == 4)
    anims::rampColorRing(ctx, stepProgress, colors::PaletteGradient<colors::White, colors::Cyan>);

  // extra display on the first pixels (count pixels to know fav no)
  if constexpr (displayFavoriteNumber)
  {
    ctx.skipFirstLedsForFrames(0);
    for (uint8_t i = 0; i < 4; ++i)
    {
      if (stepCount != 4 && i < stepCount + 1)
      {
        ctx.lamp.setPixelColor(i, colors::Cyan);
      }
      else
      {
        ctx.lamp.setPixelColor(i, colors::Black);
      }
    }
    ctx.skipFirstLedsForFrames(ctx.lamp.maxWidth * 2, 10);
  }

  // set this, after a while upon no longer holding button, favorite is set
  ctx.state.isFavoritePending = 30;
  ctx.state.whichFavoritePending = stepCount;

  // TODO: #153 remove this freeze, after migrating legacy modes :)
  ctx.skipNextFrames(10);
}

} // namespace modes::details

#endif

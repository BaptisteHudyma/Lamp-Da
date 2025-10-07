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

#include <src/system/logic/alerts.h>
#include <src/system/utils/assert.h>
#include <src/system/utils/sunset_timer.h>

#include "src/modes/include/tools.hpp"
#include "src/modes/include/context_type.hpp"
#include "src/modes/include/default_config.hpp"
#include "src/modes/include/hardware/keystore.hpp"
#include "src/modes/include/hardware/lamp_type.hpp"

#include "src/modes/include/anims/ramp_update.hpp"

namespace modes::details {

//
// \private API
//

/**
 * \brief Animate a ramp
 * \param[in,out] ctx
 * \param[in] holdDuration duration since ramp start
 * \param[in] stepSize duration of the ramp
 * \param[in] palette palette to display on the ramp, as a gradient
 * \return true if ramp arrives at the end
 */
template<bool displayFavoriteNumber = true>
bool _animate_ramp(auto& ctx, float holdDuration, float stepSize, auto palette)
{
  // where we are: 0-255 rampColorRing
  uint32_t stepProgress = floor((holdDuration * 256.0) / stepSize);
  stepProgress = stepProgress % 256;

  // only display on indexable
  if constexpr (ctx.lamp.flavor == hardware::LampTypes::indexable)
  {
    anims::rampColorRing(ctx, stepProgress, palette);
  }
  return (stepProgress >= 250);
}

/// \private display a lit pixel per given favorite index
void display_favorite_number_ramp(auto& ctx,
                                  const uint8_t favoriteIndex,
                                  const uint8_t maxFavoriteIndex,
                                  const bool display = false)
{
  ctx.skipFirstLedsForFrames(0);
  const uint8_t maxPixelDisplay = min(ctx.state.maxFavoriteCount, maxFavoriteIndex);
  for (uint8_t i = 0; i < maxPixelDisplay; ++i)
  {
    if (display and i <= favoriteIndex)
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

/// \private animate favorite picks
template<bool displayFavoriteNumber = true> void _animate_favorite_pick(auto& ctx, float holdDuration, float stepSize)
{
  // user as a number of favorite set
  // occasional +1 if not all favorite are set (allow a new favorite)
  const uint8_t numberOfFavoriteSet =
          ctx.state.usedFavoriteCount + ((ctx.state.usedFavoriteCount < ctx.state.maxFavoriteCount) ? 1 : 0);

  // up to maxFavoriteCount step state: "which_one" is [0, 1, 2, 3, ...] and "do not set" is the max index + 1
  uint32_t stepCount = numberOfFavoriteSet + floor(holdDuration / stepSize);
  stepCount = stepCount % (numberOfFavoriteSet + 1);

  // display ramp to show where user is standing
  if (stepCount >= numberOfFavoriteSet)
  {
    // cancel action
    _animate_ramp(ctx, holdDuration, stepSize, colors::PaletteGradient<colors::White, colors::Cyan>);
  }
  else
  {
    // animate with different colors
    switch (stepCount % 4)
    {
      case 0:
        _animate_ramp(ctx, holdDuration, stepSize, colors::PaletteGradient<colors::Green, colors::White>);
        break;
      case 1:
        _animate_ramp(ctx, holdDuration, stepSize, colors::PaletteGradient<colors::Blue, colors::White>);
        break;
      case 2:
        _animate_ramp(ctx, holdDuration, stepSize, colors::PaletteGradient<colors::Orange, colors::White>);
        break;
      case 3:
        _animate_ramp(ctx, holdDuration, stepSize, colors::PaletteGradient<colors::Purple, colors::White>);
        break;
    }
  }

  // extra display on the first pixels (count pixels to know fav no)
  if constexpr (displayFavoriteNumber)
  {
    // display ramp
    display_favorite_number_ramp(ctx, stepCount, numberOfFavoriteSet, stepCount < numberOfFavoriteSet);
  }

  // set this, after a while upon no longer holding button, favorite is set
  ctx.state.isFavoritePending = 10;
  ctx.state.whichFavoritePending = stepCount;

  // TODO: #153 remove this freeze, after migrating legacy modes :)
  ctx.skipNextFrames(10);
}

template<bool displayFavoriteNumber = true> void _animate_favorite_delete(auto& ctx, float holdDuration, float stepSize)
{
  // no favorite deletion if no favorites
  if (ctx.state.usedFavoriteCount <= 0)
    return;

  ctx.state.isInDeleteFavorite = true;

  uint32_t stepCount = floor(holdDuration / stepSize);
  stepCount = stepCount % 2;

  if (stepCount == 0)
  {
    if (_animate_ramp(ctx, holdDuration, stepSize, colors::PaletteGradient<colors::Orange, colors::Red>))
    {
      // set this on ramp saturation. The favorite will be removed in 2 frames
      ctx.state.isFavoriteDeletePending = 2;
    }
    else
    {
      // no deletion if release on ramp
      ctx.state.isFavoriteDeletePending = 0;
    }

    // extra display on the first pixels (count pixels to know fav no)
    if constexpr (displayFavoriteNumber)
    {
      // display ramp
      display_favorite_number_ramp(ctx, ctx.state.lastFavoriteStep, ctx.state.usedFavoriteCount, true);
    }

    // TODO: #153 remove this freeze, after migrating legacy modes :)
    ctx.skipNextFrames(10);
  }
  // else: do nothing
}

template<bool shouldDisplayRamp = true, bool shouldBlip = true>
void _animate_sunset_timer(auto& ctx, uint32_t holdDuration, float stepSize)
{
  if (_animate_ramp(ctx, holdDuration, stepSize, colors::PaletteGradient<colors::White, colors::Red>))
  {
    ctx.state.isSunsetTimingPending = 2;
  }
  else
  {
    // TODO: #153 remove this freeze, after migrating legacy modes :)
    ctx.skipNextFrames(10);
  }
}

} // namespace modes::details

namespace modes {

/// \private Active state is designated by a 32-bit integer
union ActiveIndexTy
{
  struct
  {
    /// the two index below should only be modified by set_active_group & set_active_mode
    uint8_t groupIndex; // group id (as ordered in the manager)
    uint8_t modeIndex;  // mode id (as ordered in its group)
    ///

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

  void LMBD_INLINE reset()
  {
    isForward = true;
    stepSpeed = Config::defaultCustomRampStepSpeedMs;
    rampSaturates = Config::defaultRampSaturates;
    animEffect = Config::defaultCustomRampAnimEffect;
    animChoice = Config::defaultCustomRampAnimChoice;
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

  // useful for runtime tests of mode properties
  using EveryModeBool = details::asTableFor<AllGroupsTy>;
  static constexpr auto everyBrightCallback = EveryModeBool::everyBrightCallback;
  static constexpr auto everyRequireUserThread = EveryModeBool::everyRequireUserThread;
  static constexpr auto everyCustomRamp = EveryModeBool::everyCustomRamp;
  static constexpr auto everySystemCallbacks = EveryModeBool::everySystemCallbacks;
  static constexpr auto everyButtonCustomUI = EveryModeBool::everyButtonCustomUI;

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
    usedFavoriteCount,
    favoriteModes,
    lastUsedFavorite,
    isInFavoriteGroup
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
    static constexpr uint8_t maxFavoriteCount = 8;
    std::array<ActiveIndexTy, maxFavoriteCount> favorites = {};
    uint8_t usedFavoriteCount = 0; // number of favorite set by user [0, maxFavoriteCount]

    static_assert(maxFavoriteCount < 16, "Maximum of 15 favorite as been exceeded");

    // (variables for pending favorite state machine)
    uint8_t isFavoritePending = 0;
    uint8_t whichFavoritePending = 0;
    bool isInDeleteFavorite = false;
    uint8_t isFavoriteDeletePending = 0;
    uint8_t lastFavoriteStep = 0;
    bool isInFavoriteMockGroup = false;
    uint8_t beforeFavoriteGroupIndex = 0;
    uint8_t beforeFavoriteModeIndex = 0;
    uint8_t isSunsetTimingPending = 0;

    bool isLastScrollAGroupChange = false; // last mode change in scroll changed group
    uint32_t lastScrollStopped = 0;        // keep track of the last scrool release time

    // Ramp handlers: custom ramp (or "color ramp") and mode scroll ramp
    RampHandlerTy<Config> rampHandler = {Config::defaultCustomRampStepSpeedMs};
    RampHandlerTy<Config> scrollHandler = {Config::scrollRampStepSpeedMs};

    bool clearStripOnModeChange = Config::defaultClearStripOnModeChange;

    // special effects
    uint8_t skipNextFrameEffect = 0; // should the next .loop() mode be skipped?

    // inside lamp.config
    //  - skipFirstLedsForEffect = 0; // should the loop skip some lower LEDs?
    //  - skipFirstLedsForAmount = 0; // how many pixels to shave from the top?

    // configuration-related actions done before entering mode
    static void LMBD_INLINE before_enter_mode(auto& ctx)
    {
      auto& self = ctx.state;
      self.rampHandler.reset();
      self.clearStripOnModeChange = Config::defaultClearStripOnModeChange;
    }

    // configuration-related actions done after mode entering
    static void LMBD_INLINE after_enter_mode(auto& ctx)
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
  }

  static void next_mode(auto& ctx)
  {
    dispatch_group(ctx, [](auto group) {
      group.next_mode();
    });
  }

  /// jump to an active index cleanly
  static void jump_to_new_active_index(auto& ctx, const ActiveIndexTy& newActiveIndex)
  {
    ctx.set_active_group(newActiveIndex.groupIndex, nbGroups);
    ctx.set_active_mode(newActiveIndex.modeIndex);
    // just copy the other values
    ctx.modeManager.activeIndex.customIndex = newActiveIndex.customIndex;
    ctx.modeManager.activeIndex.rampIndex = newActiveIndex.rampIndex;

    // if ramps loaded in enter_mode != ones saved as favorite, emulate update
    if (ctx.modeManager.activeIndex.rawIndex != newActiveIndex.rawIndex)
    {
      ctx.modeManager.activeIndex = newActiveIndex;
      custom_ramp_update(ctx, ctx.get_active_custom_ramp());
    }
  }

  static bool jump_to_favorite(auto& ctx, uint8_t which_one, bool shouldSaveLastActiveIndex)
  {
    // sanity check
    if (ctx.state.usedFavoriteCount <= 0)
      return false;

    // wrap back to max number of favorites
    which_one = (which_one % ctx.state.usedFavoriteCount);
    ctx.state.lastFavoriteStep = which_one;

    // store last active index before jump
    if (shouldSaveLastActiveIndex)
    {
      ctx.state.beforeFavoriteGroupIndex = ctx.modeManager.activeIndex.groupIndex;
      ctx.state.beforeFavoriteModeIndex = ctx.modeManager.activeIndex.modeIndex;
    }

    if (which_one >= ctx.state.maxFavoriteCount)
      return false;

    const auto targetFavorite = ctx.state.favorites[which_one];
    // reset once with the right mode
    jump_to_new_active_index(ctx, targetFavorite);
    return true;
  }

  static bool set_favorite_now(auto& ctx, uint8_t which_one = 0)
  {
    // new favorite added
    if (which_one != ctx.state.maxFavoriteCount && which_one == ctx.state.usedFavoriteCount)
    {
      // augment favorite count until we reach the max
      ctx.state.usedFavoriteCount = min(ctx.state.usedFavoriteCount + 1, ctx.state.maxFavoriteCount);
    }

    if (which_one < ctx.state.maxFavoriteCount)
    {
      ctx.state.favorites[which_one] = ctx.modeManager.activeIndex;
      return true;
    }
    return false;
  }

  static bool delete_favorite_now(auto& ctx)
  {
    // delete current
    const auto which_one = ctx.state.lastFavoriteStep;
    if (ctx.state.usedFavoriteCount > 0 and which_one < ctx.state.maxFavoriteCount)
    {
      ctx.state.usedFavoriteCount -= 1;

      for (uint8_t i = which_one; i < ctx.state.maxFavoriteCount - 1; ++i)
      {
        // make them all go down one spot
        ctx.state.favorites[i] = ctx.state.favorites[i + 1];
      }
      // changed favorite index, jump
      if (ctx.state.usedFavoriteCount > 0)
      {
        jump_to_new_active_index(ctx, ctx.state.favorites[which_one % ctx.state.usedFavoriteCount]);
      }
      else
      {
        // no more favorite, restore last used

        // reset favorite indicator
        ctx.state.isInFavoriteMockGroup = false;
        // return to previous state
        ctx.set_active_group(ctx.state.beforeFavoriteGroupIndex);
        ctx.set_active_mode(ctx.state.beforeFavoriteModeIndex);

        // blip to indicate favorite mode exit
        ctx.blip(250);
      }
      return true;
    }
    return false;
  }

  /// scroll through all modes
  static void handle_scroll_modes(auto& ctx, uint32_t holdDuration)
  {
    auto& scrollHandler = ctx.state.scrollHandler;
    scrollHandler.isForward = false; // (always scroll modes backward)

    static constexpr uint32_t scrollActivationTiming = 1200;
    if (holdDuration <= scrollActivationTiming)
    {
      // display the ramp and do nothing else
      modes::details::_animate_ramp(
              ctx, holdDuration, scrollActivationTiming, colors::PaletteGradient<colors::White, colors::Cyan>);

      // TODO: #153 remove this freeze, after migrating legacy modes :)
      ctx.skipNextFrames(1);
      return;
    }

    scrollHandler.update_ramp(128, holdDuration, [&](uint8_t rampValue) {
      uint8_t modeIndex = ctx.get_active_mode();
      uint8_t groupIndex = ctx.get_active_group();
      uint8_t modeCount = ctx.get_modes_count();
      uint8_t groupCount = ctx.get_groups_count();

      ctx.state.isLastScrollAGroupChange = false;

      // we are going backward
      //
      if (rampValue < 128)
      {
        // if modeIndex is not the first, just decrement it
        if (modeIndex > 0)
        {
          ctx.set_active_mode(modeIndex - 1, modeCount);

          // or else decrement group, then set mode to last one
        }
        else
        {
          ctx.state.isLastScrollAGroupChange = true;
          // if groupIndex is not the first, just decrement it
          if (groupIndex > 0)
          {
            ctx.set_active_group(groupIndex - 1, groupCount);

            // else wrap to last group
          }
          else
          {
            ctx.set_active_group(groupCount - 1, groupCount);
          }

          // backward scroll: set mode to last one on group change
          modeCount = ctx.get_modes_count();
          ctx.set_active_mode(modeCount - 1, modeCount);
        }

        // we are going forward
        //
      }
      else
      {
        // if modeIndex is not the last, just increment it
        if (modeIndex + 1 < modeCount)
        {
          ctx.next_mode();

          // or else increment group
        }
        else
        {
          ctx.state.isLastScrollAGroupChange = true;
          // if groupIndex is not the last, just increment it
          if (groupIndex + 1 < groupCount)
          {
            ctx.next_group();

            // else wrap to first group
          }
          else
          {
            ctx.set_active_group(0, groupCount);
          }

          // forward scroll: set mode to first one on group change
          ctx.set_active_mode(0, modeCount);
        }
      }
    });
  }

  static uint8_t get_modes_count(auto& ctx)
  {
    uint8_t value = 0;
    dispatch_group(ctx, [&](auto group) {
      value = decltype(group)::LocalModeTy::nbModes;
    });
    return value;
  }

  static void enter_group(auto& ctx, const uint8_t value)
  {
    auto manager = ctx.modeManager.get_context();

    // signal that we are quitting the mode
    ctx.modeManager.quit_mode(manager);

    // switch group (after quit mode)
    ctx.modeManager.activeIndex.groupIndex = value;
    // switch mode (restore last stored id)
    ctx.modeManager.activeIndex.modeIndex = ctx.state.lastModeMemory[value];

    // signal that we entered a new mode
    ctx.modeManager.enter_mode(manager);
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

  static void enter_mode(auto& ctx)
  {
    ctx.state.before_enter_mode(ctx);

    // enter mode
    dispatch_group(ctx, [](auto group) {
      group.enter_mode();
    });

    ctx.state.after_enter_mode(ctx);
  }

  static void quit_mode(auto& ctx)
  {
    dispatch_group(ctx, [](auto group) {
      group.quit_mode();
    });
  }

  //
  // all the callbacks
  //

  static void loop(auto& ctx)
  {
    // handle pending favorite
    if (ctx.state.isFavoritePending > 0)
    {
      ctx.state.isFavoritePending -= 1;

      if (ctx.state.isFavoritePending == 0 && ctx.state.whichFavoritePending <= ctx.state.usedFavoriteCount)
      {
        if (ctx.set_favorite_now(ctx.state.whichFavoritePending))
        {
          alerts::manager.raise(alerts::Type::FAVORITE_SET);
        }
      }
    }

    // handle favorite delete
    if (ctx.state.isFavoriteDeletePending > 0)
    {
      ctx.state.isFavoriteDeletePending -= 1;

      // delete favorite
      if (ctx.state.isFavoriteDeletePending == 0)
      {
        ctx.delete_favorite_now();
      }
    }

    // handle the sunset timer update
    if (ctx.state.isSunsetTimingPending > 0)
    {
      ctx.state.isSunsetTimingPending -= 1;
      if (ctx.state.isSunsetTimingPending == 0)
      {
        // set and update sunset timer
        sunset::add_time_minutes(5);
        // blip AFTER the update
        ctx.blip(50);
      }
    }

    if (ctx.lamp.config.skipFirstLedsForEffect > 0)
    {
      ctx.lamp.config.skipFirstLedsForEffect -= 1;
    }

    if (ctx.state.skipNextFrameEffect > 0)
    {
      ctx.state.skipNextFrameEffect -= 1;

      // reached last skip frame, restore mode
      if (ctx.state.skipNextFrameEffect == 0)
      {
        ctx.lamp.restoreBrightness();
      }
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

    // activate last used favorite, in the favorite group
    if (ctx.state.isInFavoriteMockGroup && jump_to_favorite(ctx, ctx.state.lastFavoriteStep, false))
    {
      // success jump to favorite
    }
    else
    {
      // activate current mode
      uint8_t groupIdBefore = ctx.get_active_group(nbGroups);
      ctx.set_active_group(groupIdBefore);
    }
  }

  static void power_off_sequence(auto& ctx)
  {
    foreach_group<true>(ctx, [](auto group) {
      group.power_off_sequence();
    });
  }

  static void write_parameters(auto& ctx)
  {
    // clear the stored parameters, before storing ours.
    store::clear_stored();

    // this scope is the only one where parameters will be kept
    ctx.template storageSaveOnly<Store::lastActive>(ctx.modeManager.activeIndex);

    // save the maxFavoriteCount possible favorites
    ctx.template storageSaveOnly<Store::usedFavoriteCount>(ctx.modeManager.state.usedFavoriteCount);
    ctx.template storageSaveOnly<Store::favoriteModes>(ctx.modeManager.state.favorites);
    ctx.template storageSaveOnly<Store::lastUsedFavorite>(ctx.modeManager.state.lastFavoriteStep);
    ctx.template storageSaveOnly<Store::isInFavoriteGroup>(ctx.state.isInFavoriteMockGroup);
    ctx.template storageSaveOnly<Store::modeMemory>(ctx.state.lastModeMemory);

    foreach_group<not hasCustomRamp>(ctx, [&ctx](auto group) {
      if constexpr (group.hasCustomRamp)
      {
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

    // load the maxFavoriteCount possible favorites
    ctx.template storageLoadOnly<Store::usedFavoriteCount>(ctx.modeManager.state.usedFavoriteCount);
    ctx.template storageLoadOnly<Store::favoriteModes>(ctx.state.favorites);
    ctx.template storageLoadOnly<Store::lastUsedFavorite>(ctx.modeManager.state.lastFavoriteStep);
    ctx.template storageLoadOnly<Store::isInFavoriteGroup>(ctx.state.isInFavoriteMockGroup);
    ctx.template storageLoadOnly<Store::modeMemory>(ctx.state.lastModeMemory);

    // for each group, migrate & handle custom ramp memory
    foreach_group<not hasCustomRamp>(ctx, [&ctx](auto group) {
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
    uint8_t groupId = ctx.get_active_group();
    uint8_t modeId = ctx.get_active_mode();

    if (ctx.everyCustomRamp[groupId][modeId] && ctx.state.rampHandler.animEffect)
      modes::anims::_rampAnimDispatch(ctx, ctx.state.rampHandler.animChoice, rampValue);

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

#endif

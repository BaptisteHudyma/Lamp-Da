/*! \file default_behavior.hpp
    \brief Define the default behavior of a lamp.
*/

#ifndef DEFAULT_BEHAVIOR_MANAGER_HPP
#define DEFAULT_BEHAVIOR_MANAGER_HPP

//
// note: this code is included as-is by:
//  - src/modes/user/indexable_behavior.hpp
//  - src/modes/user/simple_behavior.hpp
//

#include "src/system/component/time_handling.h"
#include "src/system/hal/bluetooth.h"

#include <cstdint>
namespace lampda::user {

//
// These are defined in src/modes/user/{flavor}_behavior.hpp
//

bool button_clicked_default(const uint8_t);

bool button_hold_default(const uint8_t, const bool, const uint32_t);

//
// These callbacks are the same for all lamp flavors*
//

// *meaning: LampTy is the one doing the "polyfill" between flavors

void power_on_sequence()
{
  auto manager = get_context();

  // initialize the lamp object
  manager.lamp.startup();

  // callbacks
  manager.power_on_sequence();
}

void power_off_sequence()
{
  // callbacks
  auto manager = get_context();
  manager.power_off_sequence();

  // clear lamp on power-off
  manager.lamp.clear();
  manager.lamp.show_now();

  // (no-op) internal symbol used during build
  ensure_build_canary();
}

void brightness_update(const brightness_t brightness)
{
  auto manager = get_context();

  // dont handle invalid commands
  if (brightness <= ::lampda::brightness::absoluteMaximumBrightness)
  {
    // force update of the internal references
    manager.lamp.align_internal_to_system_brightness();

    // set brightness for underlying object (w/o re-entry in update_brightness)
    manager.lamp.setBrightness(brightness, true, true);

    // callbacks
    manager.brightness_update(brightness);
  }
  else
  {
    // this call could be just a max brightness update
    manager.lamp.enforce_internal_brightness_limits();
  }
}

void sunset_timer_update(const float progress)
{
  auto manager = get_context();

  // callbacks
  manager.sunset_update(progress);
}

void write_parameters()
{
  auto manager = get_context();
  manager.write_parameters();
}

void read_parameters()
{
  auto manager = get_context();
  manager.read_parameters();
}

bool button_start_click_default(const uint8_t clicks) { return false; }

bool button_start_hold_default(const uint8_t clicks, const bool isEndOfHoldEvent, const uint32_t holdDuration)
{
  switch (clicks)
  {
    case 1:
      {
        // 1+hold (2s): activate bluetooth advertising
        // activate bluetooth !
        auto manager = get_context();
        if (not hal::bluetooth::is_activated() and
            manager.overlay_animate_ramp(
                    holdDuration, 2000, modes::colors::PaletteGradient<modes::colors::Blue, modes::colors::Blue>))
        {
          hal::bluetooth::start_advertising();
        }
        return true;
      }
    case 5:
      {
        if (not isEndOfHoldEvent and holdDuration > 0)
        {
          // sunset timer !
          auto manager = get_context();
          if (manager.overlay_animate_ramp(
                      holdDuration, 1000, modes::colors::PaletteGradient<modes::colors::White, modes::colors::Red>))
          {
            // update the sunset timing
            manager.state.isSunsetTimingPending = 2;
          }

          return true;
        }
        break;
      }
  }

  return false;
}

bool button_clicked_usermode(const uint8_t clicks)
{
  auto manager = get_context();
  return manager.custom_click(clicks);
}

bool button_hold_usermode(const uint8_t clicks, const bool isEndOfHoldEvent, const uint32_t holdDuration)
{
  auto manager = get_context();
  return manager.custom_hold(clicks, isEndOfHoldEvent, holdDuration);
}

void loop()
{
  auto manager = get_context();
  manager.loop();

  // signal display update every loop
  manager.lamp.signal_display();
}

bool should_spawn_thread()
{
#ifdef LMBD_LAMP_TYPE__INDEXABLE
  /// The thread is needed to update the strip, so non negociable.
  return true;
#else
  auto manager = get_context();
  return manager.should_spawn_thread();
#endif
}

void user_thread()
{
  auto manager = get_context();
  manager.lamp.show();

  if (manager.should_spawn_thread())
    manager.user_thread();
}

/// Define default behavior that are shared between system types
namespace default_behaviors {

/// must be called by the lampda::user::button_clicked_default
bool button_clicked(const uint8_t clicks)
{
  auto manager = get_context();

  switch (clicks)
  {
    case 6: // 6 clicks:  jump to first mode of first category
      {
        if (manager.state.isInFavoriteMockGroup)
        { // reset favorite indicator
          manager.state.isInFavoriteMockGroup = false;
        }
        // return to first state
        manager.set_active_group(0);
        manager.set_active_mode(0);
        manager.blip(250);
        return true;
      }
  }

  // nothing
  return false;
}

/// must be called by the lampda::user::button_hold_default
bool button_hold(const uint8_t clicks, const bool isEndOfHoldEvent, const uint32_t holdDuration)
{
  auto manager = get_context();
  auto& rampHandler = manager.state.rampHandler;

  switch (clicks)
  {
    case 3: // 3 click+hold: configure custom ramp
            // no ramps in favorite group
      if (not manager.state.isInFavoriteMockGroup)
      {
        rampHandler.update_ramp(manager.get_active_custom_ramp(), holdDuration, [&](uint8_t rampValue) {
          manager.custom_ramp_update(rampValue);
          manager.set_active_custom_ramp(rampValue);
        });
        return true;
      }
      break;

      // 5 click+hold: Add 5 minutes to sunset timer
    case 5:
      {
        if (not isEndOfHoldEvent and holdDuration > 0 and logic::sunset::is_enabled())
        {
          // sunset timer !
          auto manager = get_context();
          if (manager.overlay_animate_ramp(
                      holdDuration, 1000, modes::colors::PaletteGradient<modes::colors::White, modes::colors::Red>))
          {
            // update the sunset timing
            manager.state.isSunsetTimingPending = 2;
          }
        }
        break;
      }

    case 13: // 13 clicks + hold: reset the whole system and stored parameters
      {
        if (not isEndOfHoldEvent and holdDuration > 0)
        {
          auto manager = get_context();
          if (manager.overlay_animate_ramp(
                      holdDuration, 5000, modes::colors::PaletteGradient<modes::colors::Red, modes::colors::Red>))
          {
            // reset the file system and memory
            bsp::lampda_print("clearing the whole file format");
            component::fileSystem::clear_internal_fs();

            // shutdown the lamp
            const bool shouldSaveUserParameters = false;
            logic::behavior::internal::handle_shutdown_state(shouldSaveUserParameters);
          }
        }
        break;
      }

    case 20: // 20 clicks + hold: reset the whole system and stored parameters
      {
        if (not isEndOfHoldEvent and holdDuration > 0)
        {
          auto manager = get_context();
          if (manager.overlay_animate_ramp(
                      holdDuration, 5000, modes::colors::PaletteGradient<modes::colors::Red, modes::colors::Red>))
          {
            // reset the file system and memory
            bsp::lampda_print("clearing the whole file format");
            component::fileSystem::clear_internal_fs();

            // shutdown the lamp
            const bool shouldSaveUserParameters = false;
            const bool shouldSaveSystemParameters = false;
            logic::behavior::internal::handle_shutdown_state(shouldSaveUserParameters, shouldSaveSystemParameters);
          }
        }
        break;
      }
  }

  return false;
}

namespace __private {

/// handle the brightness command
void handle_brightness_control(const brightness_t requiredbrightness)
{
  // ignore if not on
  if (not logic::behavior::is_in_output_state())
    return;

  logic::sunset::lock_brightness_update(true);
  // update brightness
  const brightness_t desiredBrightness =
          min<brightness_t>(::lampda::brightness::absoluteMaximumBrightness, requiredbrightness);
  // Update the system brightness for real, not the temporary. We want the changes to be saved
  logic::brightness::update_brightness(desiredBrightness);
  // update saved brightness
  logic::brightness::update_saved_brightness();
  logic::sunset::lock_brightness_update(false);

  // and change the sunset timer if needed
  logic::sunset::bump_timer();
}

/// handle the On or Off command
void handle_on_off_command(const bool shouldBeOn)
{
  // turn on
  if (shouldBeOn)
  {
    if (not logic::behavior::is_in_output_state())
      logic::behavior::set_power_on();
  }
  // turn off
  else
  {
    if (logic::behavior::is_in_output_state())
      logic::behavior::set_power_off();
  }
}

/**
 * \brief Handle the ramp command
 * \param[in] ramp 0-255 speed
 */
void handle_ramp_command(const uint8_t ramp)
{
  // ignore if not on
  if (not logic::behavior::is_in_output_state())
    return;

  auto manager = get_context();

  // update ramp value in modes
  manager.custom_ramp_update(ramp, 1000);
  // override user choice
  manager.set_active_custom_ramp(ramp);
}

/**
 * \brief Handle the set time command
 */
void handle_set_time_command(const component::time::RealTime& time)
{
  if (not time.is_valid())
  {
    bsp::lampda_print("Refusing set_time command %d %dh %dm %ds. Invalid values.",
                      time.dayOfTheWeek,
                      time.hour,
                      time.minutes,
                      time.seconds);
    return;
  }
  const bool isValid = component::time::set_real_time(time);
  bsp::lampda_print(
          "set time %d %dh %dm %ds (validity: %d)", time.dayOfTheWeek, time.hour, time.minutes, time.seconds, isValid);
}

/**
 * \brief Handle the timing command
 */
void handle_sunset_to_time_command(const component::time::RealTime& time)
{
  if (not time.is_valid())
  {
    bsp::lampda_print("Refusing sunset command %d %dh %dm %ds. Invalid values.",
                      time.dayOfTheWeek,
                      time.hour,
                      time.minutes,
                      time.seconds);
    return;
  }
  const auto& realTime = component::time::get_real_time();
  if (not realTime.is_valid())
  {
    bsp::lampda_print("Refusing sunset command %d %dh %dm %ds : Time is not synchronized yet.",
                      time.dayOfTheWeek,
                      time.hour,
                      time.minutes,
                      time.seconds);
    return;
  }

  const uint32_t internalLampActionTime = component::time::get_platform_time_from_target_time(time);
  if (internalLampActionTime <= 0)
  {
    bsp::lampda_print("Refusing sunset command %d %dh %dm %ds: Target time is incoherent",
                      time.dayOfTheWeek,
                      time.hour,
                      time.minutes,
                      time.seconds);
    return;
  }

  // lamp will turn on if not already turned on
  if (not logic::behavior::is_in_output_state())
    logic::behavior::set_power_on();

  logic::sunset::set_deadline(internalLampActionTime);
}

void handle_mode_control(const uint8_t groupIndex, const uint8_t modeIndex)
{
  // lamp will turn on if not already turned on
  if (not logic::behavior::is_in_output_state())
    logic::behavior::set_power_on();

  auto manager = get_context();

  if (groupIndex >= manager.get_groups_count())
  {
    bsp::lampda_print("Group id %d is greater than max group id %d", groupIndex, manager.get_groups_count());
    return;
  }
  manager.set_active_group(groupIndex);

  if (modeIndex >= manager.get_modes_count())
  {
    bsp::lampda_print("Mode id %d is greater than max mode id %d", modeIndex, manager.get_modes_count());
    return;
  }
  manager.set_active_mode(modeIndex);
  manager.blip(250);
}

} // namespace __private

bool handle_user_command(const common::UserCommand& command)
{
  switch (command.get_type())
  {
    case common::UserCommand::Type::SetUserRamp:
      {
        uint8_t ramp;
        if (command.parse_ramp(ramp))
          __private::handle_ramp_command(ramp);
        else
          bsp::lampda_print("Failed to parse set_user_ramp command");
        return true;
      }
    case common::UserCommand::Type::Brightness:
      {
        brightness_t brgt;
        if (command.parse_brightness(brgt))
          __private::handle_brightness_control(brgt);
        else
          bsp::lampda_print("Failed to parse brightness command");
        return true;
      }
    case common::UserCommand::Type::SetMode:
      {
        uint8_t groupId;
        uint8_t modeId;
        if (command.parse_set_mode(groupId, modeId))
          __private::handle_mode_control(groupId, modeId);
        else
          bsp::lampda_print("Failed to parse set_mode command");
        return true;
      }
    case common::UserCommand::Type::OnOff:
      {
        bool shouldTurnOn;
        if (command.parse_turn_onoff(shouldTurnOn))
          __private::handle_on_off_command(shouldTurnOn);
        else
          bsp::lampda_print("Failed to parse onoff command");
        return true;
      }
    case common::UserCommand::Type::SetRealTime:
      {
        component::time::RealTime time;
        if (command.parse_set_real_time_command(time))
          __private::handle_set_time_command(time);
        else
          bsp::lampda_print("Failed to parse set_real_time command");
        return true;
      }
    case common::UserCommand::Type::SetSunsetToTime:
      {
        component::time::RealTime time;
        if (command.parse_set_sunset_to_time_command(time))
          __private::handle_sunset_to_time_command(time);
        else
          bsp::lampda_print("Failed to parse set_sunset_time command");
        return true;
      }

    default:
      break;
  }
  return false;
}

} // namespace default_behaviors

} // namespace lampda::user

#endif

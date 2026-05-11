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

#include <cstdint>
namespace lampda::user {

//
// These are defined in src/modes/user/{flavor}_behavior.hpp
//

void button_clicked_default(const uint8_t);

void button_hold_default(const uint8_t, const bool, const uint32_t);

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
    // this call could be just a max brigthness update
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
            platform::lampda_print("clearing the whole file format");
            physical::fileSystem::clear_internal_fs();

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
            platform::lampda_print("clearing the whole file format");
            physical::fileSystem::clear_internal_fs();

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

namespace __private_elk {

/// handle the brightness command
void handle_brigthness_control(const uint8_t requiredBrigthness)
{
  // ignore if not on
  if (not logic::behavior::is_in_output_state())
    return;

  // update brightness
  static constexpr float brightnessMultiplier = ::lampda::brightness::absoluteMaximumBrightness / 100.0f;
  const brightness_t desiredBrightness =
          min<brightness_t>(::lampda::brightness::absoluteMaximumBrightness,
                            static_cast<brightness_t>(requiredBrigthness * brightnessMultiplier));
  // Update the system brightness for real, not the temporary. We want the changes to be saved
  logic::brightness::update_brightness(desiredBrightness);
  // update saved brightness
  logic::brightness::update_saved_brightness();
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
 * \brief Handle the speed command
 * \param[in] speed 0-255 speed
 */
void handle_speed_command(const uint8_t speed)
{
  // ignore if not on
  if (not logic::behavior::is_in_output_state())
    return;

  auto manager = get_context();

  // update ramp value in modes
  manager.custom_ramp_update(speed);
  // override user choice
  manager.set_active_custom_ramp(speed);
}

} // namespace __private_elk

bool handle_elk_command(const utils::ELK::Package& elkControlCommand)
{
  switch (elkControlCommand.type)
  {
    case utils::ELK::Type::BRIGHTNESS:
      {
        __private_elk::handle_brigthness_control(elkControlCommand.data[0]);
        return true;
      }
    case utils::ELK::Type::ONOFF:
      {
        const bool shouldTurnOn = elkControlCommand.data[0] > 0;
        __private_elk::handle_on_off_command(shouldTurnOn);
        return true;
      }
    case utils::ELK::Type::PATTERN_SPEED:
      {
        const uint8_t speed = (elkControlCommand.data[0] / 100.0) * UINT8_MAX;
        __private_elk::handle_speed_command(speed);
        return true;
      }
    default:
      return false;
  }
}

} // namespace default_behaviors

} // namespace lampda::user

#endif

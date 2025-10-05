#ifndef DEFAULT_BEHAVIOR_MANAGER_HPP
#define DEFAULT_BEHAVIOR_MANAGER_HPP

//
// note: this code is included as-is by:
//  - src/modes/user/indexable_behavior.hpp
//  - src/modes/user/simple_behavior.hpp
//

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

  // set brightness for underlying object (w/o re-entry in update_brightness)
  manager.lamp.setBrightness(brightness, true, true);

  // callbacks
  manager.brightness_update(brightness);
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
      if (not isEndOfHoldEvent and holdDuration > 0)
      {
        // sunset timer !
        auto manager = get_context();
        modes::details::_animate_sunset_timer(manager, holdDuration, 1000);
        return true;
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

#endif

#ifndef BEHAVIOR_MANAGER_H
#define BEHAVIOR_MANAGER_H

//
// note: this code is included as-is by:
//  - user/indexable_functions.h
//  - simulator/src/mode-simulator.cpp
//

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
  manager.lamp.show();

  //
  // high drive input (5mA)
  // The only way to discharge the DC-DC pin...

  // (no-op) internal symbol used during build
  ensure_build_canary();
}

constexpr uint8_t minBrightness = 5;

void brightness_update(const uint8_t brightness)
{
  const uint8_t trueBrightness = max(minBrightness, brightness);
  auto manager = get_context();

  // set brightness for underlying object (w/o re-entry in update_brightness)
  manager.lamp.setBrightness(trueBrightness, true, true);

  // callbacks
  manager.brightness_update(trueBrightness);
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
  manager.reset_mode();
}

void button_clicked_default(const uint8_t clicks)
{
  auto manager = get_context();

  switch (clicks)
  {
    case 2: // 2 clicks: next mode
      manager.next_mode();
      break;

    case 3: // 3 clicks: next group
      manager.next_group();
      break;

    case 4: // 4 clicks: jump to favorite
      manager.jump_to_favorite();
      break;
  }

#ifdef LMBD_SIMU_ENABLED
  fprintf(stderr, "group %d *mode %d\n", manager.get_active_group(), manager.get_active_mode());
#endif
}

void button_hold_default(const uint8_t clicks, const bool isEndOfHoldEvent, const uint32_t holdDuration)
{
  auto manager = get_context();
  auto& rampHandler = manager.state.rampHandler;
  auto& scrollHandler = manager.state.scrollHandler;

  switch (clicks)
  {
    case 3: // 3 click+hold: configure custom ramp
      rampHandler.update_ramp(manager.get_active_custom_ramp(), holdDuration, [&](uint8_t rampValue) {
        manager.custom_ramp_update(rampValue);
        manager.set_active_custom_ramp(rampValue);
      });
      break;

    case 4: // 4 click+hold: scroll across modes and group
      scrollHandler.update_ramp(128, holdDuration, [&](uint8_t rampValue) {
        uint8_t modeIndex = manager.get_active_mode();
        uint8_t groupIndex = manager.get_active_group();
        uint8_t modeCount = manager.get_modes_count();
        uint8_t groupCount = manager.get_groups_count();

        // we are going backward
        //
        if (rampValue < 128)
        {
          // if modeIndex is not the first, just decrement it
          if (modeIndex > 0)
          {
            manager.set_active_mode(modeIndex - 1, modeCount);
            manager.reset_mode();

            // or else decrement group, then set mode to last one
          }
          else
          {
            // if groupIndex is not the first, just decrement it
            if (groupIndex > 0)
            {
              manager.set_active_group(groupIndex - 1, groupCount);

              // else wrap to last group
            }
            else
            {
              manager.set_active_group(groupCount - 1, groupCount);
            }

            // backward scroll: set mode to last one on group change
            modeCount = manager.get_modes_count();
            manager.set_active_mode(modeCount - 1, modeCount);
            manager.reset_mode();
          }

          // we are going forward
          //
        }
        else
        {
          // if modeIndex is not the last, just increment it
          if (modeIndex + 1 < modeCount)
          {
            manager.next_mode();

            // or else increment group
          }
          else
          {
            // if groupIndex is not the last, just increment it
            if (groupIndex + 1 < groupCount)
            {
              manager.next_group();

              // else wrap to first group
            }
            else
            {
              manager.set_active_group(0, groupCount);
            }

            // forward scroll: set mode to first one on group change
            manager.set_active_mode(0, modeCount);
            manager.reset_mode();
          }
        }
      });
      break;

    case 5: // 5 click+hold: configure favorite

      // TODO: add button animation to help know when favorite is set
      if (holdDuration > 2000)
      {
        manager.set_favorite_now();
      }

      break;

    default:
      break;
  }
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
}

bool should_spawn_thread() { return true; }

void user_thread()
{
  auto manager = get_context();
  manager.lamp.show();

  if (manager.should_spawn_thread())
  {
    manager.user_thread();
  }
}

#endif

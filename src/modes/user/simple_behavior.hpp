/*! \file simple_behavior.hpp
    \brief Define the behavior of an simple lamp.
*/

#ifndef SIMPLE_BEHAVIOR_MANAGER_H
#define SIMPLE_BEHAVIOR_MANAGER_H

//
// note: this code is included as-is by:
//  - user/simple_functions.h
//

namespace lampda::user {

bool button_clicked_default(const uint8_t clicks)
{
  // Handle default common behavior
  if (default_behaviors::button_clicked(clicks))
  {
    // some event is already handled
    return true;
  }

  auto manager = get_context();

  switch (clicks)
  {
    case 2:
      {
        if (manager.get_active_group() == 0)
        {
          // if at max brightness, go to saved brightness
          if (manager.lamp.getBrightness() == manager.lamp.getMaxBrightness())
          {
            manager.lamp.restoreBrightness();
          }
          // else set max brightness
          else
          {
            manager.lamp.tempBrightness(manager.lamp.getMaxBrightness());
          }
        }
        else
        {
          manager.next_mode();

#ifdef LMBD_SIMULATION
          fprintf(stderr, "group %d *mode %d\n", manager.get_active_group(), manager.get_active_mode());
#endif
        }
        return true;
      }

    case 3:
      {
        // 3C at max brightness will produce a light boost (dangerous for the strip if held for a long time)
        if (manager.get_active_group() == 0 and
            // check that brightness is at the absolute maximum level
            manager.lamp.getBrightness() == ::lampda::brightness::absoluteMaximumBrightness)
        {
          // write a power boost (dangerous) for a limited time
          physical::outputPower::write_temporary_output_limits(::lampda::stripInputMaxVoltage_mV * 1.2f, 5000, 5000);
        }
        else
        {
          manager.next_group();

#ifdef LMBD_SIMULATION
          fprintf(stderr, "group %d *mode %d\n", manager.get_active_group(), manager.get_active_mode());
#endif
        }
        return true;
      }

    default:
      break;
  }
  return false;
}

bool button_hold_default(const uint8_t clicks, const bool isEndOfHoldEvent, const uint32_t holdDuration)
{
  // Handle default common behavior
  if (default_behaviors::button_hold(clicks, isEndOfHoldEvent, holdDuration))
  {
    // some event is already handled
    return true;
  }

  auto manager = get_context();
  auto& rampHandler = manager.state.rampHandler;

  switch (clicks)
  {
    // No special behavior
    default:
      break;
  }
  return false;
}

void handle_elk_command(const utils::ELK::Package& elkControlCommand)
{
  // Handle default common behavior
  if (default_behaviors::handle_elk_command(elkControlCommand))
  {
    // some event is already handled
    return;
  }
}

} // namespace lampda::user

#endif

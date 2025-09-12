#ifndef SIMPLE_BEHAVIOR_MANAGER_H
#define SIMPLE_BEHAVIOR_MANAGER_H

//
// note: this code is included as-is by:
//  - user/simple_functions.h
//

void button_clicked_default(const uint8_t clicks)
{
  auto manager = get_context();

  switch (clicks)
  {
    case 2:
      if (manager.get_active_group() == 0)
      {
        // if at max brightness, go to saved brightness
        if (manager.lamp.getBrightness() == maxBrightness)
        {
          manager.lamp.restoreBrightness();
        }
        // else set max brightness
        else
        {
          manager.lamp.tempBrightness(maxBrightness);
        }
      }
      else
        manager.next_mode();
      break;

    case 3:
      manager.next_group();
      break;

    default:
      break;
  }

#ifdef LMBD_SIMULATION
  fprintf(stderr, "group %d *mode %d\n", manager.get_active_group(), manager.get_active_mode());
#endif
}

void button_hold_default(const uint8_t clicks, const bool isEndOfHoldEvent, const uint32_t holdDuration)
{
  switch (clicks)
  {
    case 3:
      // no-op
      break;
    default:
      break;
  }
}

#endif

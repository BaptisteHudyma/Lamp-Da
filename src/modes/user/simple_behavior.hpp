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
      // 3C at max brightness will produce a light boost (dangerous for the strip if held for a long time)
      if (manager.get_active_group() == 0 and manager.lamp.getBrightness() == maxBrightness)
      {
        // write a power boost (dangerous) for a limited time
        outputPower::write_temporary_output_limits(inputVoltage_V * 1000 * 1.2f, 5000, 5000);
      }
      else
      {
        manager.next_group();
      }
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
  auto manager = get_context();
  auto& rampHandler = manager.state.rampHandler;

  switch (clicks)
  {
    case 3: // 3 click+hold: configure custom ramp
      rampHandler.update_ramp(manager.get_active_custom_ramp(), holdDuration, [&](uint8_t rampValue) {
        manager.custom_ramp_update(rampValue);
        manager.set_active_custom_ramp(rampValue);
      });
      break;
    default:
      break;
  }
}

#endif

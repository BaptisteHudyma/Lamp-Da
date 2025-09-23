#ifndef INDEXABLE_BEHAVIOR_MANAGER_H
#define INDEXABLE_BEHAVIOR_MANAGER_H

//
// note: this code is included as-is by:
//  - user/indexable_functions.h
//

void button_clicked_default(const uint8_t clicks)
{
  auto manager = get_context();

  switch (clicks)
  {
    case 2: // 2 clicks: next mode
      if (manager.state.isInFavoriteMockGroup)
      {
        // if in favorite, next favorite
        manager.state.lastFavoriteStep += 1;
        // sanity check, if it fails, quit favorites
        manager.state.isInFavoriteMockGroup = manager.jump_to_favorite(manager.state.lastFavoriteStep, false);
      }
      else
      {
        // else: just change mode
        manager.next_mode();
      }
      break;

    case 3: // 3 clicks: next group or quit favorite group
      if (manager.state.isInFavoriteMockGroup)
      {
#ifdef LMBD_SIMULATION
        fprintf(stderr, "Exit fake favorite group\n");
#endif
        // reset favorite indicator
        manager.state.isInFavoriteMockGroup = false;
        // return to previous state
        manager.set_active_group(manager.state.beforeFavoriteGroupIndex);
        manager.set_active_mode(manager.state.beforeFavoriteModeIndex);

        // blip to indicate favorite mode exit
        manager.blip(250);
      }
      else
      {
        // true next group
        manager.next_group();
      }
      break;

    case 4: // 4 clicks: jump to favorite

      if (not manager.state.isInFavoriteMockGroup)
      {
        // jump and save last used mode
        if (manager.jump_to_favorite(manager.state.lastFavoriteStep, true))
        {
          manager.state.isInFavoriteMockGroup = true;
          // blip to indicate favorite mode enter
          manager.blip(250);
        }
      }
      break;
  }

#ifdef LMBD_SIMULATION
  {
    int _groupId = manager.get_active_group();
    int _modeId = manager.get_active_mode();
    fprintf(stderr, "group %d *mode %d\n", _groupId, _modeId);
    if (manager.everyBrightCallback[_groupId][_modeId])
      fprintf(stderr, " - hasBrightCallback\n");
    if (manager.everyCustomRamp[_groupId][_modeId])
      fprintf(stderr, " - hasCustomRamp\n");
    if (manager.state.rampHandler.animEffect)
      fprintf(stderr, " - has anim effect\n");
    if (manager.everyRequireUserThread[_groupId][_modeId])
      fprintf(stderr, " - requireUserThread\n");
    if (manager.everySystemCallbacks[_groupId][_modeId])
      fprintf(stderr, " - hasSystemCallbacks\n");
    if (manager.everyButtonCustomUI[_groupId][_modeId])
      fprintf(stderr, " - hasButtonCustomUI\n");
  }
#endif
}

void button_hold_default(const uint8_t clicks, const bool isEndOfHoldEvent, const uint32_t holdDuration)
{
  auto manager = get_context();
  auto& rampHandler = manager.state.rampHandler;
  auto& scrollHandler = manager.state.scrollHandler;
  scrollHandler.isForward = false; // (always scroll modes backward)

  switch (clicks)
  {
    case 3: // 3 click+hold: configure custom ramp
      rampHandler.update_ramp(manager.get_active_custom_ramp(), holdDuration, [&](uint8_t rampValue) {
        manager.custom_ramp_update(rampValue);
        manager.set_active_custom_ramp(rampValue);
      });
      break;

    case 4: // 4 click+hold: scroll across modes and group
      // no scroll in favorite
      if (not manager.state.isInFavoriteMockGroup)
      {
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
            }
          }
        });
      }
      break;

    case 5: // 5 click+hold: configure favorite
      // no new favorite in favorite
      if (not manager.state.isInFavoriteMockGroup)
      {
        modes::details::_animate_favorite_pick(manager, holdDuration, 2000);
      }
      break;

    default:
      break;
  }
}

#endif

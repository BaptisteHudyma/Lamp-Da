#include "sunset_timer.h"

#include "src/system/logic/alerts.h"
#include "src/system/logic/behavior.h"
#include "src/system/logic/brightness_handle.h"

#include "src/system/platform/print.h"
#include "src/system/platform/threads.h"
#include "src/system/platform/time.h"

#include "src/system/utils/utils.h"

namespace lampda {
namespace logic {
namespace sunset {

uint32_t sunsetTimerEndTime_s = 0;
bool isAllowedToControlBrightness = true;

static constexpr uint32_t brightnessRampDownTime_min = 3;
static constexpr uint32_t brightnessRampDownTime_s = brightnessRampDownTime_min * 60;
static constexpr uint32_t brightnessRampDownTime_ms = brightnessRampDownTime_s * 1000;
static constexpr uint16_t brightnessDecreasePerLoop = 1;

// minimum calls to the update sunset function that will be made
static constexpr uint16_t minimalSunsetUpdateCalls = 50;

// sunset loop time to reduce brightness gradually
uint32_t get_sunset_loop_timing_ms()
{
  const auto& maxBrightnessStep = logic::brightness::get_saved_brightness() / brightnessDecreasePerLoop;
  if (maxBrightnessStep <= 0)
    return 100;
  const uint32_t res = (brightnessRampDownTime_ms / maxBrightnessStep) + 1;
  if (res <= 5)
    return 5;
  // minimum turn off delay, to prevent too slow turn off at low luminosities
  return min<uint32_t>(res, brightnessRampDownTime_ms / minimalSunsetUpdateCalls);
}

/// Return the percent of advance of the sunset timer, from 0 to 1. 1 is end of process.
float get_percent_of_advance()
{
  if (platform::time_s() >= sunsetTimerEndTime_s)
    return 1.0;

  const uint32_t finishline = get_sunset_loop_timing_ms();

  // signal the progress change
  const float progress = lmpd_constrain<float>(((sunsetTimerEndTime_s * 1000.0 - finishline) - platform::time_ms()) /
                                                       static_cast<float>(brightnessRampDownTime_ms),
                                               0.0f,
                                               1.0f);
  return 1.0 - progress;
}

/// Send the timer update signal to consummers
/// Returns the progress
float signal_sunset_update()
{
  if (platform::time_s() >= sunsetTimerEndTime_s)
  {
    logic::behavior::sunset::progress_update(1.0f);
    return 1.0;
  }

  const float progress = get_percent_of_advance();
  logic::behavior::sunset::progress_update(progress);
  return progress;
}

void sunset_process_loop()
{
  // this thread runs slowly
  platform::delay_ms(get_sunset_loop_timing_ms());

  if (sunsetTimerEndTime_s == 0)
  {
    // sunset time not set, auto suspend
    platform::threads::suspend_this_thread();
  }
  else
  {
    // less than N minutes, start to decrease brightness
    if (platform::time_s() + brightnessRampDownTime_s >= sunsetTimerEndTime_s)
    {
      // signal the progress change
      const float progress = signal_sunset_update();
      if (progress >= 1.0)
      {
        logic::brightness::set_max_user_brightness(0);
        logic::brightness::force_brightness_user_callback();
        platform::lampda_print("Shutdown with sunset timer");
        logic::behavior::set_power_off();
        cancel_timer();
        return;
      }
      else
      {
        if (isAllowedToControlBrightness)
        {
          // new brightness to use
          const brightness_t newBrightness =
                  lmpd_constrain<float>(1.0 - progress, 0.0, 1.0) * logic::brightness::get_saved_brightness();

          // slowly decrease brighntess
          logic::brightness::set_max_user_brightness(newBrightness);
          // force an update of the brightness, with user callback
          logic::brightness::force_brightness_user_callback();
        }
      }
    }
  }
}

void init()
{
  // start in suspended mode
  platform::threads::start_suspended_thread(sunset_process_loop, platform::threads::sunset_taskName, 0, 1024);
}

void add_time_minutes(const uint8_t time_minutes)
{
  // do not accept
  if (time_minutes < 1)
    return;

  const uint32_t timeToAdd_s = min<uint8_t>(10, time_minutes) * 60;
  if (sunsetTimerEndTime_s == 0)
  {
    platform::lampda_print("sunset timer set");
    sunsetTimerEndTime_s = platform::time_s() + timeToAdd_s;
    logic::alerts::manager.raise(logic::alerts::Type::SUNSET_TIMER_ENABLED);

    // resume
    platform::threads::resume_thread(platform::threads::sunset_taskName);
  }
  else
  {
    platform::lampda_print("sunset timer add %d minutes", (timeToAdd_s / 60));
    sunsetTimerEndTime_s += timeToAdd_s;

    // added some time, so signal update
    signal_sunset_update();
  }

  // restore stored brightness as the user limit
  logic::brightness::set_max_user_brightness(logic::brightness::get_saved_brightness());
}

/// signal to the timer that some time must be added. Limited to 10 minutes
void bump_timer()
{
  const auto timeS = platform::time_s();
  if (sunsetTimerEndTime_s == 0 or sunsetTimerEndTime_s < timeS)
    return;

  // if less than N minutes left, bump timer to N minutes
  if (sunsetTimerEndTime_s - timeS < brightnessRampDownTime_s)
  {
    // add 1 minute + time left
    const uint16_t timeLeft_min = round((sunsetTimerEndTime_s - timeS) / 60.0);
    // add some time to the sunset
    add_time_minutes(1 + (brightnessRampDownTime_min - timeLeft_min));
  }
}

/// cancel the current active timer
void cancel_timer()
{
  const bool isSunsetActive = sunsetTimerEndTime_s != 0;
  // release timer
  sunsetTimerEndTime_s = 0;
  lock_brightness_update(false);

  logic::brightness::set_max_user_brightness(logic::brightness::get_max_brightness());
  // signal change
  if (isSunsetActive)
  {
    signal_sunset_update();
    platform::lampda_print("sunset timer cleared");
  }

  logic::alerts::manager.clear(logic::alerts::Type::SUNSET_TIMER_ENABLED);
}

bool is_enabled() { return sunsetTimerEndTime_s > 0; }

void lock_brightness_update(bool shouldLock) { isAllowedToControlBrightness = not shouldLock; }

} // namespace sunset
} // namespace logic
} // namespace lampda

#include "sunset_timer.h"

#include "src/system/hal/time.h"

#include "src/system/bsp/text_out.h"
#include "src/system/bsp/threads.h"

#include "src/system/component/time_handling.h"

#include "src/system/logic/alerts.h"
#include "src/system/logic/behavior.h"
#include "src/system/logic/brightness_handle.h"

#include "src/system/utils/utils.h"

namespace lampda {
namespace logic {
namespace sunset {

volatile uint32_t sunsetTimerEndTime_s = 0;
bool isAllowedToControlBrightness = true;

/// Indicate if the sunset is set
bool is_enabled() { return sunsetTimerEndTime_s > 0; }

static constexpr uint32_t brightnessRampDownTime_min = 3;
static constexpr uint32_t brightnessRampDownTime_s = brightnessRampDownTime_min * 60;
static constexpr uint32_t brightnessRampDownTime_ms = brightnessRampDownTime_s * 1000;
static constexpr uint16_t brightnessDecreasePerLoop = 1;

/// minimum calls to the update sunset function that will be made
static constexpr uint16_t minimalSunsetUpdateCalls = 50;
/// Minimum time between sunset loop calls
static constexpr uint16_t minimalSunsetLoopDuration_ms = 5;

// sunset loop time to reduce brightness gradually
uint32_t get_sunset_loop_timing_ms()
{
  const auto& maxBrightnessStep = logic::brightness::get_saved_brightness() / brightnessDecreasePerLoop;
  if (maxBrightnessStep <= 0)
    return 100;
  const uint32_t res = (brightnessRampDownTime_ms / maxBrightnessStep) + 1;
  if (res <= minimalSunsetLoopDuration_ms)
    return minimalSunsetLoopDuration_ms;
  // minimum turn off delay, to prevent too slow turn off at low luminosities
  return min<uint32_t>(res, brightnessRampDownTime_ms / minimalSunsetUpdateCalls);
}

/// Return the percent of advance of the sunset timer, from 0 to 1. 1 is end of process.
float get_percent_of_advance()
{
  if (hal::time_s() >= sunsetTimerEndTime_s)
    return 1.0;

  const uint32_t finishline = get_sunset_loop_timing_ms();

  // signal the progress change
  const float progress = lmpd_constrain<float>(((sunsetTimerEndTime_s * 1000.0 - finishline) - hal::time_ms()) /
                                                       static_cast<float>(brightnessRampDownTime_ms),
                                               0.0f,
                                               1.0f);
  return 1.0 - progress;
}

/// Send the timer update signal to consummers
/// Returns the progress
float signal_sunset_update()
{
  if (hal::time_s() >= sunsetTimerEndTime_s)
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
  hal::delay_ms(get_sunset_loop_timing_ms());

  if (not is_enabled())
  {
    return;
  }
  // less than N minutes, start to decrease brightness
  if (hal::time_s() + brightnessRampDownTime_s >= sunsetTimerEndTime_s)
  {
    // signal the progress change
    const float progress = signal_sunset_update();
    if (progress >= 1.0)
    {
      cancel_timer();

      logic::brightness::set_max_user_brightness(0);
      logic::brightness::force_brightness_user_callback();

      bsp::lampda_print("Shutdown with sunset timer");
      logic::behavior::set_power_off();
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

void init()
{
  // start in suspended mode
  bsp::threads::start_suspended_thread(sunset_process_loop, bsp::threads::sunset_taskName, 0, 255);
}

void set_deadline(const uint32_t timeshutdown_s)
{
  if (timeshutdown_s <= hal::time_s())
  {
    bsp::lampda_print("shutdown time is less than current time: %d", timeshutdown_s);
    return;
  }
  const uint32_t timeLeftMinutes = round((timeshutdown_s - hal::time_s()) / 60);

  const auto& shutdownTime = component::time::convert_to_real_time(timeshutdown_s);
  if (shutdownTime.is_valid())
  {
    bsp::lampda_print("lamp will auto turn off on %d %dh %dm %ds",
                      shutdownTime.dayOfTheWeek,
                      shutdownTime.hour,
                      shutdownTime.minutes,
                      shutdownTime.seconds);
  }

  if (not is_enabled())
  {
    bsp::lampda_print("sunset timer set to %d minutes", timeLeftMinutes);
    sunsetTimerEndTime_s = timeshutdown_s;
    logic::alerts::manager.raise(logic::alerts::Type::SUNSET_TIMER_ENABLED);

    // resume
    bsp::threads::resume_thread(bsp::threads::sunset_taskName);
  }
  else
  {
    bsp::lampda_print("sunset timer updated to %d minutes", timeLeftMinutes);
    // added some time, so signal update
    sunsetTimerEndTime_s = timeshutdown_s;
    signal_sunset_update();
  }

  // restore stored brightness to the user limit
  logic::brightness::set_max_user_brightness(logic::brightness::get_saved_brightness());
}

void add_time_minutes(const uint8_t time_minutes)
{
  // do not accept less than a minute
  if (time_minutes < 1)
    return;
  const auto timeS = hal::time_s();

  // limit 10 minutes per call
  const uint32_t timeToAdd_s = min<uint32_t>(10, time_minutes) * 60;

  if (is_enabled() && timeS < sunsetTimerEndTime_s)
  {
    const uint32_t newShutdownTime_s = sunsetTimerEndTime_s + timeToAdd_s;
    set_deadline(newShutdownTime_s);
  }
  else
  {
    const uint32_t newShutdownTime_s = timeS + timeToAdd_s;
    set_deadline(newShutdownTime_s);
  }
}

void shortcut_to_phaseout()
{
  // sunset is enabled
  if (is_enabled())
  {
    const uint32_t minimumEndTime_s = hal::time_s() + brightnessRampDownTime_s;
    // Never extend a timer that is already in the phaseout phase
    if (sunsetTimerEndTime_s <= minimumEndTime_s)
      return;

    set_deadline(minimumEndTime_s);
  }
}

void bump_timer()
{
  if (not is_enabled())
    return;

  const auto timeS = hal::time_s();
  if (sunsetTimerEndTime_s < timeS)
  {
    add_time_minutes(brightnessRampDownTime_min);
    return;
  }

  // if less than N minutes left, bump timer to N minutes
  const uint16_t timeLeft_min = round((sunsetTimerEndTime_s - timeS) / 60.0f);
  if (timeLeft_min <= brightnessRampDownTime_min)
  {
    // add 1 minute + time left
    const uint16_t timeLeft = brightnessRampDownTime_min - timeLeft_min;
    // add some time to the sunset
    add_time_minutes(1 + timeLeft);
  }
}

/// cancel the current active timer
void cancel_timer()
{
  // release timer
  sunsetTimerEndTime_s = 0;
  lock_brightness_update(false);

  logic::brightness::set_max_user_brightness(logic::brightness::get_max_brightness());
  // signal change
  if (is_enabled())
  {
    signal_sunset_update();
    bsp::lampda_print("sunset timer cleared");
  }

  logic::alerts::manager.clear(logic::alerts::Type::SUNSET_TIMER_ENABLED);
}

void lock_brightness_update(bool shouldLock) { isAllowedToControlBrightness = not shouldLock; }

} // namespace sunset
} // namespace logic
} // namespace lampda

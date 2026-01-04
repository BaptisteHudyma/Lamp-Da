#include "sunset_timer.h"

#include "brightness_handle.h"
#include "utils.h"

#include "src/system/logic/alerts.h"
#include "src/system/logic/behavior.h"

#include "src/system/platform/threads.h"
#include "src/system/platform/print.h"
#include "src/system/platform/time.h"

namespace sunset {

uint32_t sunsetTimerEndTime_s = 0;

static constexpr uint32_t brightnessRampDownTime_min = 3;
static constexpr uint32_t brightnessRampDownTime_s = brightnessRampDownTime_min * 60;
static constexpr uint32_t brightnessRampDownTime_ms = brightnessRampDownTime_s * 1000;
static constexpr uint16_t brightnessDecreasePerLoop = 1;

// minimum calls to the update sunset function that will be made
static constexpr uint16_t minimalSunsetUpdateCalls = 50;

const char* const sunset_taskName = "sunset";

void signal_sunset_update()
{
  if (time_s() > sunsetTimerEndTime_s)
  {
    behavior::sunset::progress_update(1.0f);
    return;
  }

  // signal the progress change
  const float progress = lmpd_constrain<float>(
          (sunsetTimerEndTime_s - time_s()) / static_cast<float>(brightnessRampDownTime_s), 0.0f, 1.0f);
  behavior::sunset::progress_update(1.0f - progress);
}

// sunset loop time to reduce brightness gradually
uint32_t get_sunset_loop_timing_ms()
{
  const auto& maxBrightnessStep = brightness::get_saved_brightness() / brightnessDecreasePerLoop;
  if (maxBrightnessStep <= 0)
    return 100;
  const uint32_t res = (brightnessRampDownTime_ms / maxBrightnessStep) + 1;
  if (res <= 5)
    return 5;
  // minimum turn off delay, to prevent too slow turn off at low luminosities
  return min<uint32_t>(res, brightnessRampDownTime_ms / minimalSunsetUpdateCalls);
}

void sunset_process_loop()
{
  // this thread runs very slowly
  delay_ms(get_sunset_loop_timing_ms());

  if (sunsetTimerEndTime_s == 0)
  {
    // sunset time not set, auto suspend
    suspend_this_thread();
  }
  else
  {
    // less than N minutes, start to decrease brightness
    if (time_s() + brightnessRampDownTime_s >= sunsetTimerEndTime_s)
    {
      // signal the progress change
      signal_sunset_update();

      // decrease brightness every N seconds left
      const auto currentBrightness = brightness::get_brightness();
      if (time_s() >= sunsetTimerEndTime_s)
      {
        brightness::update_brightness(0);
        lampda_print("Shutdown with sunset timer");
        behavior::set_power_off();
        cancel_timer();
        return;
      }
      else if (currentBrightness >= brightnessDecreasePerLoop)
      {
        // slowly decrease brighntess
        brightness::update_brightness(currentBrightness - brightnessDecreasePerLoop);
      }
      // else: casual loop update
    }
  }
}

void init()
{
  // start in suspended mode
  start_suspended_thread(sunset_process_loop, sunset_taskName, 0, 1024);
}

void add_time_minutes(const uint8_t time_minutes)
{
  // do not accept
  if (time_minutes < 1)
    return;

  const uint32_t timeToAdd_s = min<uint8_t>(10, time_minutes) * 60;
  if (sunsetTimerEndTime_s == 0)
  {
    lampda_print("sunset timer set");
    sunsetTimerEndTime_s = time_s() + timeToAdd_s;
    alerts::manager.raise(alerts::Type::SUNSET_TIMER_ENABLED);

    // resume
    resume_thread(sunset_taskName);
  }
  else
  {
    lampda_print("sunset timer add %d minutes", (timeToAdd_s / 60));
    sunsetTimerEndTime_s += timeToAdd_s;

    // added some time, so signal update
    signal_sunset_update();
  }
}

/// signal to the timer that some time must be added. Limited to 10 minutes
void bump_timer()
{
  if (sunsetTimerEndTime_s == 0 or sunsetTimerEndTime_s < time_s())
    return;

  // if less than 3 minutes left, bump timer to N minutes
  if (sunsetTimerEndTime_s - time_s() < brightnessRampDownTime_s)
  {
    sunsetTimerEndTime_s = time_s() + (brightnessRampDownTime_min + 1) * 60;
  }
}

/// cancel the current active timer
void cancel_timer()
{
  // release timer
  sunsetTimerEndTime_s = 0;
  lampda_print("sunset timer cleared");
  alerts::manager.clear(alerts::Type::SUNSET_TIMER_ENABLED);
}

bool is_enabled() { return sunsetTimerEndTime_s > 0; }

} // namespace sunset

#include "sunrise_timer.h"

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
namespace sunrise {

volatile uint32_t sunriseTimerEndTime_s = 0;
bool isAllowedToControlBrightness = true;

static constexpr uint32_t brightnessRampUpTime_min = 3;
static constexpr uint32_t brightnessRampUpTime_s = brightnessRampUpTime_min * 60;
static constexpr uint32_t brightnessRampUpTime_ms = brightnessRampUpTime_s * 1000;
static constexpr uint16_t brightnessIncreasePerLoop = 1;

/// This can be used to set a specific increase time
uint32_t trueBrightnessRampUpTime_ms = brightnessRampUpTime_ms;

/// minimum calls to the update sunrise function that will be made
static constexpr uint16_t minimalSunriseUpdateCalls = 50;
/// Minimum time between sunrise loop calls
static constexpr uint16_t minimalSunriseLoopDuration_ms = 5;

namespace __private {

uint32_t get_sunrise_loop_timing_ms()
{
  const auto& maxBrightnessStep = logic::brightness::get_saved_brightness() / brightnessIncreasePerLoop;
  if (maxBrightnessStep <= 0)
    return 100;
  const uint32_t res = (trueBrightnessRampUpTime_ms / maxBrightnessStep) + 1;
  if (res <= minimalSunriseLoopDuration_ms)
    return minimalSunriseLoopDuration_ms;
  // minimum turn on delay, to prevent too slow turn on at low luminosities
  return min<uint32_t>(res, trueBrightnessRampUpTime_ms / minimalSunriseUpdateCalls);
}

/// Return the percent of advance of the sunset timer, from 0 to 1. 1 is end of process.
float get_percent_of_advance()
{
  if (hal::time_s() >= sunriseTimerEndTime_s)
    return 1.0f;

  const uint32_t finishline = get_sunrise_loop_timing_ms();

  // signal the progress change
  const float progress = lmpd_constrain<float>(((sunriseTimerEndTime_s * 1000.0f - finishline) - hal::time_ms()) /
                                                       static_cast<float>(trueBrightnessRampUpTime_ms),
                                               0.0f,
                                               1.0f);
  return 1.0f - progress;
}

/// Send the timer update signal to consummers
/// Returns the progress
float signal_sunrise_update()
{
  if (hal::time_s() >= sunriseTimerEndTime_s)
  {
    logic::behavior::timers::sunrise_progress_update(1.0f);
    return 1.0f;
  }

  const float progress = get_percent_of_advance();
  logic::behavior::timers::sunrise_progress_update(progress);
  return progress;
}

void sunrise_process_loop()
{
  if (not is_enabled())
  {
    hal::delay_ms(100);
    return;
  }

  // this thread runs slowly
  hal::delay_ms(get_sunrise_loop_timing_ms());

  // less than N minutes, start to increase brightness
  if (hal::time_s() + brightnessRampUpTime_s >= sunriseTimerEndTime_s)
  {
    // signal the progress change
    const float progress = signal_sunrise_update();
    if (progress >= 1.0)
    {
      cancel_timer();

      logic::brightness::set_max_user_brightness(logic::brightness::get_saved_brightness());
      // force an update of the brightness, with user callback
      logic::brightness::force_brightness_user_callback();

      bsp::lampda_print("Sunrise timer finished !");

      // autostop: finished  !
      return;
    }
    else
    {
      // timing is reaching the end phase, power on the lamp
      if (not logic::behavior::is_in_output_state())
      {
        logic::brightness::set_max_user_brightness(0);
        logic::behavior::set_power_on();
      }

      if (isAllowedToControlBrightness)
      {
        // new brightness to use
        const float constraintProgress = lmpd_constrain<float>(progress, 0.0f, 1.0f);
        const brightness_t newMaxBrightness = constraintProgress * logic::brightness::get_max_brightness();
        const brightness_t newBrightness = constraintProgress * logic::brightness::get_saved_brightness();

        // slowly increase brightness limit
        logic::brightness::set_max_user_brightness(newMaxBrightness);
        // update brightness
        logic::brightness::update_brightness(newBrightness);
      }
    }
  }
}

} // namespace __private

void init()
{ // start in suspended mode
  static constexpr uint16_t sunriseThreadBufferSize = 255;
  static bsp::threads::TaskBuffer_t sunriseTaskBuffer[sunriseThreadBufferSize];
  bsp::threads::start_suspended_thread(
          __private::sunrise_process_loop, bsp::threads::sunrise_taskName, 0, 255, sunriseTaskBuffer);
}

void set_deadline(const uint32_t timeToFullyOn_s)
{
  if (timeToFullyOn_s <= hal::time_s())
  {
    bsp::lampda_print("shutdown time is less than current time: %d", timeToFullyOn_s);
    return;
  }
  if (not is_enabled() and logic::behavior::is_in_output_state())
  {
    bsp::lampda_print("Sunrise mode can only be used with an off lamp");
    return;
  }
  const uint32_t timeLeftSeconds = timeToFullyOn_s - hal::time_s();
  const uint32_t timeLeftMinutes = round(timeLeftSeconds / 60);

  const auto& shutdownTime = component::time::convert_to_real_time(timeToFullyOn_s);
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
    bsp::lampda_print("sunrise timer set to %d minutes", timeLeftMinutes);
    sunriseTimerEndTime_s = timeToFullyOn_s;
    logic::alerts::manager.raise(logic::alerts::Type::SUNSET_TIMER_ENABLED);

    // resume
    bsp::threads::resume_thread(bsp::threads::sunrise_taskName);
  }
  else
  {
    bsp::lampda_print("sunrise timer updated to %d minutes", timeLeftMinutes);
    // added some time, so signal update
    sunriseTimerEndTime_s = timeToFullyOn_s;
    __private::signal_sunrise_update();
  }

  // update the true end time, in case the standard was bypassed
  if (timeLeftSeconds < brightnessRampUpTime_s)
    trueBrightnessRampUpTime_ms = timeLeftSeconds * 1000;
  else
    trueBrightnessRampUpTime_ms = brightnessRampUpTime_ms;
}

void add_time_minutes(const uint8_t time_minutes)
{
  // do not accept less than a minute
  if (time_minutes < 1)
    return;
  const auto timeS = hal::time_s();

  // limit 10 minutes per call
  const uint32_t timeToAdd_s = min<uint32_t>(10, time_minutes) * 60;

  if (is_enabled() && timeS < sunriseTimerEndTime_s)
  {
    const uint32_t newFullyOnTime_s = sunriseTimerEndTime_s + timeToAdd_s;
    set_deadline(newFullyOnTime_s);
  }
  else
  {
    const uint32_t newFullyOnTime_s = timeS + timeToAdd_s;
    set_deadline(newFullyOnTime_s);
  }
}

void cancel_timer()
{
  if (is_enabled())
    logic::brightness::set_max_user_brightness(logic::brightness::get_max_brightness());

  // release timer
  sunriseTimerEndTime_s = 0;
  trueBrightnessRampUpTime_ms = brightnessRampUpTime_ms;
  lock_brightness_update(false);

  logic::alerts::manager.clear(logic::alerts::Type::SUNSET_TIMER_ENABLED);
}

bool is_enabled() { return sunriseTimerEndTime_s > 0; }

void lock_brightness_update(bool shouldLock) { isAllowedToControlBrightness = not shouldLock; }

} // namespace sunrise
} // namespace logic
} // namespace lampda

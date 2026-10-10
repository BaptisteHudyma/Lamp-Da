#include "sun_timers.h"

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

template<uint32_t T> void SunTimerBase<T>::cancel()
{
  if (is_enabled())
    logic::brightness::set_max_user_brightness(logic::brightness::get_max_brightness());

  // release timer
  endTime_s = 0;
  trueBrightnessSunTime_ms = BrightnessSunTime_ms;

  lock_brightness_update(false);
  logic::alerts::manager.clear(logic::alerts::Type::SUNSET_TIMER_ENABLED);
}

template<uint32_t T> void SunTimerBase<T>::set_deadline(const uint32_t timeshutdown_s)
{
  if (timeshutdown_s <= hal::time_s())
  {
    bsp::lampda_print("timer end time is less than current time: %d", timeshutdown_s);
    return;
  }
  if (not should_accept_start())
  {
    bsp::lampda_print("Sun timer cannot be used currently");
    return;
  }

  const uint32_t timeLeftSeconds = timeshutdown_s - hal::time_s();
  const uint32_t timeLeftMinutes = round(timeLeftSeconds / 60);

  const auto& shutdownTime = component::time::convert_to_real_time(timeshutdown_s);
  if (shutdownTime.is_valid())
  {
    bsp::lampda_print("timer will finish on %d %dh %dm %ds",
                      shutdownTime.dayOfTheWeek,
                      shutdownTime.hour,
                      shutdownTime.minutes,
                      shutdownTime.seconds);
  }

  if (not is_enabled())
  {
    bsp::lampda_print("sun timer set to %d minutes", timeLeftMinutes);
    endTime_s = timeshutdown_s;
    logic::alerts::manager.raise(logic::alerts::Type::SUNSET_TIMER_ENABLED);

    // resume
    bsp::threads::resume_thread(get_task_identifier());
  }
  else
  {
    bsp::lampda_print("sun timer updated to %d minutes", timeLeftMinutes);
    // added some time, so signal update
    endTime_s = timeshutdown_s;
    signal_sun_update();
  }

  // update the true end time, in case the standard was bypassed
  if (timeLeftSeconds < BrightnessSunTime_s)
    trueBrightnessSunTime_ms = timeLeftSeconds * 1000;
  else
    trueBrightnessSunTime_ms = BrightnessSunTime_ms;

  // restore stored brightness to the user limit
  /// TODO: logic::brightness::set_max_user_brightness(logic::brightness::get_saved_brightness());
}

template<uint32_t T> void SunTimerBase<T>::add_time_minutes(const uint8_t time_minutes)
{
  // do not accept less than a minute
  if (time_minutes < 1)
    return;
  const auto timeS = hal::time_s();

  // limit 10 minutes per call
  const uint32_t timeToAdd_s = min<uint32_t>(10, time_minutes) * 60;

  if (is_enabled() && timeS < endTime_s)
  {
    const uint32_t newShutdownTime_s = endTime_s + timeToAdd_s;
    set_deadline(newShutdownTime_s);
  }
  else
  {
    const uint32_t newShutdownTime_s = timeS + timeToAdd_s;
    set_deadline(newShutdownTime_s);
  }
}

template<uint32_t T> uint32_t SunTimerBase<T>::get_loop_timing_ms() const
{
  const auto& maxBrightnessStep = logic::brightness::get_saved_brightness() / brightnessIncreasePerLoop;
  if (maxBrightnessStep <= 0)
    return 100;
  const uint32_t res = (trueBrightnessSunTime_ms / maxBrightnessStep) + 1;
  if (res <= minimalSunLoopDuration_ms)
    return minimalSunLoopDuration_ms;
  // minimum turn on delay, to prevent too slow turn on at low luminosities
  return min<uint32_t>(res, trueBrightnessSunTime_ms / minimalSunUpdateCalls);
}

template<uint32_t T> float SunTimerBase<T>::get_percent_of_advance() const
{
  if (hal::time_s() >= endTime_s)
    return 1.0f;

  const uint32_t finishline = get_loop_timing_ms();

  // signal the progress change
  const float progress = lmpd_constrain<float>(((endTime_s * 1000.0f - finishline) - hal::time_ms()) /
                                                       static_cast<float>(trueBrightnessSunTime_ms),
                                               0.0f,
                                               1.0f);
  return 1.0f - progress;
}

template<uint32_t T> float SunTimerBase<T>::signal_sun_update() const
{
  if (hal::time_s() >= endTime_s)
  {
    sun_timer_progress_update_callback(1.0f);
    return 1.0f;
  }

  const float progress = get_percent_of_advance();
  sun_timer_progress_update_callback(progress);
  return progress;
}

template<uint32_t T> void SunTimerBase<T>::shortcut_to_brightness_phase()
{
  // sunset is enabled
  if (is_enabled())
  {
    const uint32_t minimumEndTime_s = hal::time_s() + BrightnessSunTime_s;
    // Never extend a timer that is already in the phaseout phase
    if (endTime_s <= minimumEndTime_s)
      return;

    set_deadline(minimumEndTime_s);
  }
}

template<uint32_t T> void SunTimerBase<T>::sun_process_loop()
{
  if (not is_enabled())
  {
    hal::delay_ms(100);
    return;
  }

  // this thread runs slowly
  hal::delay_ms(get_loop_timing_ms());

  // less than N minutes, start to control brightness
  if (hal::time_s() + BrightnessSunTime_s >= endTime_s)
  {
    // signal the progress change
    const float progress = signal_sun_update();
    if (progress >= 1.0)
    {
      cancel();

      end_of_timer_handle();

      // autostop: finished  !
      return;
    }
    else
    {
      timer_brightness_progress_update(lmpd_constrain<float>(progress, 0.0f, 1.0f));
    }
  }
}

// Explicit template instantiation
template class SunTimerBase<3>;

/**
 *
 *   OBJECTS
 *
 */

SunsetTimer sunset_timer;
SunriseTimer sunrise_timer;

/**
 *
 *   SUNSET
 *
 */

void SunsetTimer::init()
{
  // start in suspended mode
  static constexpr uint16_t sunsetThreadBufferSize = 255;
  static bsp::threads::TaskBuffer_t sunsetTaskBuffer[sunsetThreadBufferSize];

  bsp::threads::start_suspended_thread(&SunsetTimer::loop_task, get_task_identifier(), 0, 255, sunsetTaskBuffer);
}

void SunsetTimer::loop_task() { sunset_timer.sun_process_loop(); }

uint32_t SunsetTimer::get_task_identifier() const { return bsp::threads::sunset_taskName; }

void SunsetTimer::bump_timer()
{
  if (not is_enabled())
    return;

  const auto timeS = hal::time_s();
  if (endTime_s < timeS)
  {
    add_time_minutes(BrightnessSunTime_min);
    return;
  }

  // if less than N minutes left, bump timer to N minutes
  const uint16_t timeLeft_min = round((endTime_s - timeS) / 60.0f);
  if (timeLeft_min <= BrightnessSunTime_min)
  {
    // add 1 minute + time left
    const uint16_t timeLeft = BrightnessSunTime_min - timeLeft_min;
    // add some time to the timer
    add_time_minutes(1 + timeLeft);
  }
}

void SunsetTimer::sun_timer_progress_update_callback(const float progress) const
{
  logic::behavior::timers::sunset_progress_update(progress);
}

void SunsetTimer::end_of_timer_handle()
{
  logic::brightness::set_max_user_brightness(0);
  logic::brightness::force_brightness_user_callback();

  bsp::lampda_print("Shutdown with sunset timer");
  logic::behavior::set_power_off();
}

void SunsetTimer::timer_brightness_progress_update(const float progress)
{
  if (isAllowedToControlBrightness)
  {
    // new brightness to use
    const brightness_t newBrightness = (1.0f - progress) * logic::brightness::get_saved_brightness();

    // slowly decrease brighntess
    logic::brightness::set_max_user_brightness(newBrightness);
    // force an update of the brightness, with user callback
    logic::brightness::force_brightness_user_callback();
  }
}

/**
 *
 *   SUNRISE
 *
 */

void SunriseTimer::init()
{
  // start in suspended mode
  static constexpr uint16_t sunriseThreadBufferSize = 255;
  static bsp::threads::TaskBuffer_t sunriseTaskBuffer[sunriseThreadBufferSize];

  bsp::threads::start_suspended_thread(&SunriseTimer::loop_task, get_task_identifier(), 0, 255, sunriseTaskBuffer);
}

void SunriseTimer::loop_task() { sunrise_timer.sun_process_loop(); }

uint32_t SunriseTimer::get_task_identifier() const { return bsp::threads::sunrise_taskName; }

bool SunriseTimer::should_accept_start() const
{
  // only accept updates if we are already enabled, or we can start (output is off)
  return is_enabled() or not logic::behavior::is_in_output_state();
}

void SunriseTimer::sun_timer_progress_update_callback(const float progress) const
{
  logic::behavior::timers::sunrise_progress_update(progress);
}

void SunriseTimer::end_of_timer_handle()
{
  logic::brightness::set_max_user_brightness(logic::brightness::get_saved_brightness());
  // force an update of the brightness, with user callback
  logic::brightness::force_brightness_user_callback();

  bsp::lampda_print("Sunrise timer finished !");
}

void SunriseTimer::timer_brightness_progress_update(const float progress)
{
  // timing is reaching the end phase, power on the system
  if (not logic::behavior::is_in_output_state())
  {
    logic::brightness::set_max_user_brightness(0);
    logic::behavior::set_power_on();
  }

  if (isAllowedToControlBrightness)
  {
    // new brightness to use
    const brightness_t newMaxBrightness = progress * logic::brightness::get_max_brightness();
    const brightness_t newBrightness = progress * logic::brightness::get_saved_brightness();

    // slowly increase brightness limit
    logic::brightness::set_max_user_brightness(newMaxBrightness);
    // update brightness
    logic::brightness::update_brightness(newBrightness);
  }
}

} // namespace logic
} // namespace lampda

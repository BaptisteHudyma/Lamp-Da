/**
 * \file sun_timers.h
 * \brief Handle the sun timers: sunset to auto shutdown the system and sunrise for auto turn on.
 */

#pragma once

#include <cstdint>

namespace lampda {
namespace logic {

/// Sun timer operations handler
/// All sun operations are exclusive, thay cannot run at the same time
template<uint32_t BrightnessSunTime> class SunTimerBase
{
public:
  static constexpr uint32_t BrightnessSunTime_min = BrightnessSunTime;
  static constexpr uint32_t BrightnessSunTime_s = BrightnessSunTime_min * 60;
  static constexpr uint32_t BrightnessSunTime_ms = BrightnessSunTime_s * 1000;
  static constexpr uint16_t brightnessDecreasePerLoop = 1;

  /// Brightness increase per loop call during the brightness phase
  static constexpr uint16_t brightnessIncreasePerLoop = 1;
  /// minimum calls to the update sunset function that will be made
  static constexpr uint16_t minimalSunUpdateCalls = 50;
  /// Minimum time between sunset loop calls
  static constexpr uint16_t minimalSunLoopDuration_ms = 5;

  virtual void init() = 0;

  bool is_enabled() const { return endTime_s > 0; }

  /// Cancel the current active timer
  void cancel();

  /// Start the sun timer with a deadline, or prolonge the current sun timer
  void set_deadline(const uint32_t timeshutdown_s);

  /// Add some time to the current timer, or start it with a target time in minutes.
  /// This time cannot be shorter than BrightnessSunTime_min
  void add_time_minutes(const uint8_t time_minutes);

  /// Lock the hability of the sunset timer to control the brightness
  void lock_brightness_update(bool shouldLock) { isAllowedToControlBrightness = not shouldLock; }

  /// Shortcut the whole timer to the brightness control phase
  void shortcut_to_brightness_phase();

  /// sun process task
  /// INTERNAL USE ONLY
  void sun_process_loop();

protected:
  /// Return the identifier of the task to use
  virtual uint32_t get_task_identifier() const = 0;

  /// Overloadable: A timer can refuse to start under some conditions
  virtual bool should_accept_start() const { return true; };

  /// Wrapper to the custom user code to call for a sun timer update
  /// \param[in] pogress between 0 and 1
  virtual void sun_timer_progress_update_callback(const float progress) const = 0;

  /// Called when the timer it's target
  virtual void end_of_timer_handle() = 0;

  /// Called when the timer progresses through the brightness phase
  virtual void timer_brightness_progress_update(const float progress) = 0;

protected:
  /// Compute the timing delay of the loop to stay at the target brightness control frequency
  uint32_t get_loop_timing_ms() const;

  /// Return the sun timer progress between 0 to 1.
  float get_percent_of_advance() const;

  /// Signal a sun timer update to consummers.
  /// \return  the progress between 0 and 1
  float signal_sun_update() const;

  /// End of the timer operation, in the lamp time counter
  uint32_t endTime_s = 0;
  /// Is the handler allowed to control lamp brightness
  bool isAllowedToControlBrightness = true;

private:
  /// Duration of the sun rise/set during wich the brightness will be controled
  uint32_t trueBrightnessSunTime_ms = BrightnessSunTime_ms;
};

/// Timer to slowly ramp down the brightness until zero then shutdown.
class SunsetTimer : public SunTimerBase<3>
{
public:
  // Call once on program start
  void init() override;

  /**
   * \brief signal to the timer that some time may be added.
   * Only add time if the timer is in the fadeout phase
   */
  void bump_timer();

protected:
  static void loop_task();

  uint32_t get_task_identifier() const override;

  void sun_timer_progress_update_callback(const float progress) const override;

  void end_of_timer_handle() override;

  void timer_brightness_progress_update(const float progress) override;
};

/// Timer to start the system and ramp up brightness until we reach back the latest user set brightness
class SunriseTimer : public SunTimerBase<3>
{
public:
  // Call once on program start
  void init() override;

protected:
  static void loop_task();

  uint32_t get_task_identifier() const override;

  bool should_accept_start() const override;

  void sun_timer_progress_update_callback(const float progress) const override;

  void end_of_timer_handle() override;

  void timer_brightness_progress_update(const float progress) override;
};

/// object to access to control the timers
extern SunsetTimer sunset_timer;
extern SunriseTimer sunrise_timer;

/// init all timers
inline void sun_timers_init_all()
{
  sunset_timer.init();
  sunrise_timer.init();
}

inline void sun_timers_cancel_all()
{
  sunset_timer.cancel();
  sunrise_timer.cancel();
}

inline void sun_timers_brightness_lock_all(const bool shouldLock)
{
  sunset_timer.lock_brightness_update(shouldLock);
  sunrise_timer.lock_brightness_update(shouldLock);
}

} // namespace logic
} // namespace lampda

#ifdef LMBD_LAMP_TYPE__SIMPLE

#include <cstdint>

#include "src/system/behavior.h"

#include "src/system/utils/utils.h"
#include "src/system/utils/brightness_handle.h"
#include "src/system/utils/curves.h"
#include "src/system/utils/print.h"

#include "src/system/platform/time.h"

#include "src/system/physical/fileSystem.h"
#include "src/system/physical/output_power.h"
#include "src/system/physical/imu.h"

#include "src/user/functions.h"

namespace user {

void power_on_sequence()
{
  brightness_update(brightness::get_brightness());

  imu::enable_interrupt_1(imu::EventType::BigMotion);
}

void power_off_sequence()
{
#ifdef LMBD_CPP17
  ensure_build_canary(); // (no-op) internal symbol used during build
#endif
}

void brightness_update(const brightness_t brightness)
{
  const brightness_t constraintBrightness = min(brightness, maxBrightness);

  if (constraintBrightness >= maxBrightness)
  {
    // blip
    outputPower::blip();
  }

  // map to a new curve, favorising low levels
  static constexpr uint16_t maxOutputVoltage_mV = inputVoltage_V * 1000;
  using curve_t = curves::ExponentialCurve<brightness_t, uint16_t>;
  static curve_t brightnessCurve(
          curve_t::point_t {0, 9400}, curve_t::point_t {maxBrightness, maxOutputVoltage_mV}, 1.0);

  outputPower::write_voltage(round(brightnessCurve.sample(constraintBrightness)));
}

void write_parameters() {}

void read_parameters() {}

void button_clicked_default(const uint8_t clicks)
{
  switch (clicks)
  {
    // put luminosity to maximum
    case 2:
      brightness::update_brightness(maxBrightness);
      brightness::update_previous_brightness();
      break;

    default:
      break;
  }
}

void button_hold_default(const uint8_t, const bool, const uint32_t) {}

bool button_clicked_usermode(const uint8_t) { return usermodeDefaultsToLockdown; }

bool button_hold_usermode(const uint8_t, const bool, const uint32_t) { return usermodeDefaultsToLockdown; }

void loop()
{
  static uint32_t event_raised_time = 0;
  static brightness_t savedBrightness = 0;

  // if (imu::is_event_detected(imu::EventType::FreeFall))
  if (imu::is_event_detected(imu::EventType::BigMotion))
  // if (imu::is_interrupt1_enabled())
  {
    brightness_update(0);
    savedBrightness = brightness::get_brightness();
    event_raised_time = time_ms();
  }

  if (event_raised_time != 0 and time_ms() - event_raised_time > 300)
  {
    brightness_update(savedBrightness);
    event_raised_time = 0;
  }
}

bool should_spawn_thread() { return false; }

void user_thread() {}

} // namespace user

#endif

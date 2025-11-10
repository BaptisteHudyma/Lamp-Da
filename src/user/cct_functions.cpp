#ifdef LMBD_LAMP_TYPE__CCT

#include <cstdint>

#include "src/system/logic/behavior.h"

#include "src/system/utils/utils.h"
#include "src/system/utils/curves.h"
#include "src/system/utils/brightness_handle.h"

#include "src/system/physical/fileSystem.h"
#include "src/system/physical/output_power.h"

#include "src/system/platform/gpio.h"
#include "src/system/platform/time.h"

#include "src/user/functions.h"

namespace user {

constexpr uint8_t minBrightness = 13;

static DigitalPin WhiteColorPin(DigitalPin::GPIO::gpio6);
static DigitalPin YellowColorPin(DigitalPin::GPIO::gpio7);

constexpr uint32_t colorKey = utils::hash("color");
uint8_t currentColor = 0;
uint8_t lastColor = 0;

static uint8_t currentBrightness = 0;

void set_color(const uint8_t color)
{
  const float reduced = currentBrightness / 255.0;

  uint8_t yellowColor = (UINT8_MAX - color) * reduced;
  uint8_t whiteColor = color * reduced;

  YellowColorPin.write(yellowColor);
  WhiteColorPin.write(whiteColor);
}

void power_on_sequence()
{
  YellowColorPin.set_pin_mode(DigitalPin::Mode::kOutput);
  WhiteColorPin.set_pin_mode(DigitalPin::Mode::kOutput);

  brightness_update(brightness::get_brightness());
}

void power_off_sequence()
{
  // reset the output
  currentBrightness = 0;
  set_color(0);

#ifdef LMBD_CPP17
  ensure_build_canary(); // (no-op) internal symbol used during build
#endif
}

void brightness_update(const brightness_t brightness)
{
  const brightness_t constraintBrightness = min<brightness_t>(brightness, maxBrightness);

  if (constraintBrightness == maxBrightness)
  {
    // blip
    outputPower::blip(50);
  }

  // map to a new curve, favorising low levels
  using curve_t = curves::ExponentialCurve<brightness_t, uint8_t>;
  static curve_t brightnessCurve(curve_t::point_t {0, minBrightness}, curve_t::point_t {maxBrightness, 255}, 50.0);

  currentBrightness = round(brightnessCurve.sample(constraintBrightness));

  outputPower::write_voltage(stripInputVoltage_mV);
  set_color(currentColor);
}

void write_parameters() { fileSystem::user::set_value(colorKey, currentColor); }

void read_parameters()
{
  uint32_t mode = 0;
  if (fileSystem::user::get_value(colorKey, mode))
  {
    currentColor = mode;
    lastColor = currentColor;
  }

  currentBrightness = brightness::get_brightness();
}

bool button_start_click_default(const uint8_t clicks) { return false; }

bool button_start_hold_default(const uint8_t clicks, const bool isEndOfHoldEvent, const uint32_t holdDuration)
{
  return false;
}

void button_clicked_default(const uint8_t clicks)
{
  switch (clicks)
  {
    // put luminosity to maximum
    case 2:
      brightness::update_brightness(maxBrightness);
      break;

    default:
      break;
  }
}

void button_hold_default(const uint8_t clicks, const bool isEndOfHoldEvent, const uint32_t holdDuration)
{
  constexpr uint32_t COLOR_RAMP_DURATION_MS = 4000;
  switch (clicks)
  {
    case 3: // 3 clicks and hold
      if (!isEndOfHoldEvent)
      {
        const float percentOfTimeToGoUp = float(UINT8_MAX - lastColor) / (float)UINT8_MAX;
        currentColor = lmpd_map<uint8_t>(min<uint32_t>(holdDuration, COLOR_RAMP_DURATION_MS * percentOfTimeToGoUp),
                                         0,
                                         COLOR_RAMP_DURATION_MS * percentOfTimeToGoUp,
                                         lastColor,
                                         UINT8_MAX);
      }
      else
      {
        lastColor = currentColor;
      }
      break;

    case 4:
      if (!isEndOfHoldEvent)
      {
        const double percentOfTimeToGoDown = float(lastColor) / (float)UINT8_MAX;

        currentColor = lmpd_map<uint8_t>(min<uint32_t>(holdDuration, COLOR_RAMP_DURATION_MS * percentOfTimeToGoDown),
                                         0,
                                         COLOR_RAMP_DURATION_MS * percentOfTimeToGoDown,
                                         lastColor,
                                         0);
      }
      else
      {
        lastColor = currentColor;
      }
      break;
  }
}

bool button_clicked_usermode(const uint8_t) { return usermodeDefaultsToLockdown; }

bool button_hold_usermode(const uint8_t, const bool, const uint32_t) { return usermodeDefaultsToLockdown; }

void loop() { set_color(currentColor); }

bool should_spawn_thread() { return false; }

void user_thread() {}

} // namespace user

#endif

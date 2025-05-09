#ifndef PHYSICAL_INDICATOR_CPP
#define PHYSICAL_INDICATOR_CPP

#include "indicator.h"

#include "src/system/utils/constants.h"
#include "src/system/utils/utils.h"
#include "src/system/platform/time.h"

#include "src/system/platform/gpio.h"
#include "src/system/utils/input_output.h"

namespace indicator {

// Pins for the led on the button
static DigitalPin ButtonRedPin(RedIndicator);
static DigitalPin ButtonGreenPin(GreenIndicator);
static DigitalPin ButtonBluePin(BlueIndicator);

void init()
{
  ButtonRedPin.set_pin_mode(DigitalPin::Mode::kOutput);
  ButtonBluePin.set_pin_mode(DigitalPin::Mode::kOutput);
  ButtonGreenPin.set_pin_mode(DigitalPin::Mode::kOutput);

  set_color(utils::ColorSpace::BLACK);
}

void set_color(utils::ColorSpace::RGB c)
{
  static constexpr float redColorCorrection = 1.0;
  static constexpr float greenColorCorrection =
          1.0 / 7.5; // the green of this button is way way higher than the other colors
  static constexpr float blueColorCorrection = 1.0;

  const COLOR& col = c.get_rgb();
  ButtonRedPin.write(col.red * redColorCorrection);
  ButtonGreenPin.write(col.green * greenColorCorrection);
  ButtonBluePin.write(col.blue * blueColorCorrection);
}

bool breeze(const uint32_t periodOn, const uint32_t periodOff, const utils::ColorSpace::RGB& color)
{
  const uint32_t time = time_ms();

  // store the start time of the animation
  static uint32_t startTime = time;
  static bool isOn = false;

  // breeze on
  const uint32_t timeSinceStart = time - startTime;
  if (timeSinceStart < periodOn)
  {
    isOn = true;
    const float progression = lmpd_map<uint32_t, float>(timeSinceStart, 0, periodOn, 0.0, 1.0);

    // rising edge
    if (progression <= 0.5)
    {
      // map from [0.0; 0.5] to [0.0; 1.0]
      set_color(utils::ColorSpace::RGB(utils::get_gradient(0, color.get_rgb().color, 2.0 * progression)));
    }
    // falling edge
    else
    {
      // map from ]0.5; 1.0] to [0.0; 1.0]
      set_color(utils::ColorSpace::RGB(utils::get_gradient(0, color.get_rgb().color, 2.0 * (1.0 - progression))));
    }
  }
  // breeze off
  else if (timeSinceStart < periodOn + periodOff)
  {
    isOn = false;
    set_color(utils::ColorSpace::BLACK);
  }
  else
  {
    isOn = false;
    // reset animation
    startTime = time;
    set_color(utils::ColorSpace::BLACK);
  }

  return not isOn;
}

bool blink(const uint32_t offFreq, const uint32_t onFreq, utils::ColorSpace::RGB color)
{
  static uint32_t lastCall = 0;
  static bool ledState = false;

  // led is off, and last call was some delay before
  if (not ledState and time_ms() - lastCall > onFreq)
  {
    ledState = true;
    set_color(color);
    lastCall = time_ms();
  }

  // led is on, and last call was long ago
  if (ledState and time_ms() - lastCall > offFreq)
  {
    ledState = false;
    // set black
    set_color(utils::ColorSpace::RGB(0));
    lastCall = time_ms();
  }

  return not ledState;
}

} // namespace indicator

#endif
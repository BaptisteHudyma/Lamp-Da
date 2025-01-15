#ifndef PHYSICAL_INDICATOR_CPP
#define PHYSICAL_INDICATOR_CPP

#include "indicator.h"

#include "src/system/utils/constants.h"
#include "src/system/utils/utils.h"
#include "src/system/platform/time.h"

#include "src/system/platform/gpio.h"

namespace indicator {

// Pins for the led on the button
static DigitalPin ButtonRedPin(DigitalPin::GPIO::p8);
static DigitalPin ButtonGreenPin(DigitalPin::GPIO::p4);
static DigitalPin ButtonBluePin(DigitalPin::GPIO::p7);

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

  // breeze on
  const uint32_t timeSinceStart = time - startTime;
  if (timeSinceStart < periodOn)
  {
    const float progression = utils::map(timeSinceStart, 0, periodOn, 0.0, 1.0);

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
    set_color(utils::ColorSpace::BLACK);
  }
  else
  {
    // reset animation
    startTime = time;
    set_color(utils::ColorSpace::BLACK);
  }

  return false;
}

bool blink(const uint32_t offFreq, const uint32_t onFreq, utils::ColorSpace::RGB color)
{
  static uint32_t lastCall = 0;
  static bool ledState = false;

  // led is off, and last call was 100ms before
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

  return false;
}

} // namespace indicator

#endif
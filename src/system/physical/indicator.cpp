#ifndef PHYSICAL_INDICATOR_CPP
#define PHYSICAL_INDICATOR_CPP

#include "indicator.h"

#include "src/system/utils/constants.h"
#include "src/system/utils/utils.h"
#include "src/system/utils/input_output.h"
#include "src/system/utils/time_utils.h"

#include "src/system/platform/time.h"
#include "src/system/platform/gpio.h"

namespace indicator {

static_assert(redColorCorrection * 255 > 32, "red correction is too small to be visible");
static_assert(greenColorCorrection * 255 > 32, "green correction is too small to be visible");
static_assert(blueColorCorrection * 255 > 32, "blue correction is too small to be visible");

// Pins for the led on the button
static const DigitalPin ButtonRedPin(RedIndicator);
static const DigitalPin ButtonGreenPin(GreenIndicator);
static const DigitalPin ButtonBluePin(BlueIndicator);

static inline float brigthnessMultiplier = 1.0f;

void init()
{
  ButtonRedPin.set_pin_mode(DigitalPin::Mode::kOutput);
  ButtonBluePin.set_pin_mode(DigitalPin::Mode::kOutput);
  ButtonGreenPin.set_pin_mode(DigitalPin::Mode::kOutput);

  set_color(utils::ColorSpace::BLACK);
}

void set_color(const utils::ColorSpace::RGB& c)
{
  const COLOR& col = c.get_rgb();
  ButtonRedPin.write(std::ceil(col.red * redColorCorrection * brigthnessMultiplier));
  ButtonGreenPin.write(std::ceil(col.green * greenColorCorrection * brigthnessMultiplier));
  ButtonBluePin.write(std::ceil(col.blue * blueColorCorrection * brigthnessMultiplier));
}

void set_brightness(const uint8_t brightness) { brigthnessMultiplier = brightness / 255.0f; }
uint8_t get_brightness() { return brigthnessMultiplier * 255; }

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
    const float progression =
            lmpd_constrain(lmpd_map<uint32_t, float>(timeSinceStart, 0, periodOn, 0.0f, 1.0f), 0.0f, 1.0f);

    // rising edge
    if (progression <= 0.5)
    {
      // map from [0.0; 0.5] to [0.0; 1.0]
      set_color(utils::ColorSpace::RGB(utils::get_gradient(0, color.get_rgb().color, 2.0f * progression)));
    }
    // falling edge
    else
    {
      // map from ]0.5; 1.0] to [0.0; 1.0]
      set_color(utils::ColorSpace::RGB(utils::get_gradient(0, color.get_rgb().color, 2.0f * (1.0f - progression))));
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

bool color_loop(const uint32_t colorDuration, std::initializer_list<utils::ColorSpace::RGB> colors)
{
  static size_t currentColorIndex = 0;

  // last call was some delay before (with a refresh of the delay)
  EVERY_N_MILLIS_REFRESH(colorDuration)
  {
    set_color(*(colors.begin() + currentColorIndex));

    // increase index, limit to colors size
    currentColorIndex = (currentColorIndex + 1) % colors.size();
  }

  return currentColorIndex == 0;
}

bool blink(const uint32_t offFreq, const uint32_t onFreq, std::initializer_list<utils::ColorSpace::RGB> colors)
{
  static uint32_t lastCall = 0;
  static bool ledState = false;
  static size_t currentColorIndex = 0;

  // led is off, and last call was some delay before
  if (not ledState and time_ms() - lastCall > onFreq)
  {
    // increase index, limit to colors size
    currentColorIndex = (currentColorIndex + 1) % colors.size();

    ledState = true;
    set_color(*(colors.begin() + currentColorIndex));
    lastCall = time_ms();
  }

  // led is on, and last call was long ago
  if (ledState and time_ms() - lastCall > offFreq)
  {
    ledState = false;
    // set black
    set_color(utils::ColorSpace::BLACK);
    lastCall = time_ms();
  }

  return not ledState;
}

} // namespace indicator

#endif

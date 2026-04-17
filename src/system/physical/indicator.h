/*! \file indicator.h
    \brief Interface for the physical components of the RGB user indicator.
*/

#ifndef PHYSICAL_INDICATOR_H
#define PHYSICAL_INDICATOR_H

#include "src/system/utils/colorspace.h"
#include <initializer_list>

/// Handle the RGB indicator display.
namespace indicator {

// RGB leds do not have the same output power per color
/// Scaling for the red color channel of an this RGB diode
static constexpr float redColorCorrection = 1.0f;
/// Scaling for the green color channel of an this RGB diode
static constexpr float greenColorCorrection = 1.0f / 3.0f;
/// Scaling for the blue color channel of an this RGB diode
static constexpr float blueColorCorrection = 1.0f / 4.0f;

/// Initialize the RGB indicator. Call once on program start
extern void init();

/// Set the color of the indicator
extern void set_color(const utils::ColorSpace::RGB& c);

/// Set an indicator brightness (0-255)
extern void set_brightness(const uint8_t brightness);
/// Return the brightness of the indicator (0-255)
extern uint8_t get_brightness();

/**
 * \brief Breeze animation
 * \param[in] periodOn in mS, on time
 * \param[in] periodOff in mS, off time
 * \param[in] color The color of the animation
 * \return true when the animation finished. It loops automatically if called again
 */
extern bool breeze(const uint32_t periodOn, const uint32_t periodOff, const utils::ColorSpace::RGB& color);

/**
 * \brief color loop animation
 * \param[in] colorDuration in mS, color time
 * \param[in] colors The colors of the animation, colors will cycle between on/off cycles
 * \return true when the animation finished. It loops automatically if called again
 */
extern bool color_loop(const uint32_t colorDuration, std::initializer_list<utils::ColorSpace::RGB> colors);

/**
 * \brief Blink animation
 * \param[in] periodOn in mS, on time
 * \param[in] periodOff in mS, off time
 * \param[in] colors The colors of the animation, colors will cycle between on/off cycles
 * \return true when the animation finished. It loops automatically if called again
 */
extern bool blink(const uint32_t offFreq, const uint32_t onFreq, std::initializer_list<utils::ColorSpace::RGB> colors);

/**
 * \brief Blink animation
 * \param[in] periodOn in mS, on time
 * \param[in] periodOff in mS, off time
 * \param[in] color The color of the animation
 * \return true when the animation finished. It loops automatically if called again
 */
inline bool blink(const uint32_t offFreq, const uint32_t onFreq, const utils::ColorSpace::RGB& color)
{
  return blink(offFreq, onFreq, {color});
}

} // namespace indicator

#endif

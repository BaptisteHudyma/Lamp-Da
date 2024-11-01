#ifndef ANIMATIONS_ANIMATIONS_H
#define ANIMATIONS_ANIMATIONS_H

#ifdef LMBD_LAMP_TYPE__INDEXABLE

#include "src/system/utils/colorspace.h"
#include "src/system/utils/strip.h"

#include "colors.h"

namespace animations {

/**
 * \brief Fill the display with a color, with an optional cutoff value
 * \param[in] color class that returns a color to display
 * \param[in, out] strip The led strip to control
 * \param[in] cutOff between 0 and 1, how much this gradient will fill the
 * display before suddently cutting of
 */
void fill(const Color& color, LedStrip& strip, const float cutOff = 1);

/**
 * \brief Do a wipe down followed by a wipe up animation
 * \param[in] color class that returns a color to display
 * \param[in] duration The duration of the animation, in milliseconds
 * \param[in] fadeOut The animation fade speed (0: no fade)
 * \param[in] restart If true, the animation will restart
 * \param[in, out] strip The led strip to control
 * \param[in] cutOff between 0 and 1, how much this gradient will fill the
 * display before suddently cutting of \return True if the animation is finished
 */
bool dot_ping_pong(const Color& color, const uint32_t duration,
                   const uint8_t fadeOut, const bool restart, LedStrip& strip,
                   const float cutOff = 1);

/**
 * \brief Do color pulse
 * \param[in] color class that returns a color to display
 * \param[in] durationPulseUp The duration of the color fill up animation, in
 * milliseconds \param[in] durationPulseDown The duration of the pong animation,
 * in milliseconds \param[in] restart If true, the animation will restart
 * \param[in, out] strip The led strip to control
 * \param[in] cutOff between 0 and 1, how much this color will fill the display
 * before suddently cutting of \return True if the animation is finished
 */
bool color_pulse(const Color& color, const uint32_t durationPulseUp,
                 const uint32_t durationPulseDown, const bool restart,
                 LedStrip& strip, const float cutOff = 1);

/**
 * \brief Fill the display from both side simultaneously
 * \param[in] color class that returns a color to display
 * \param[in] duration The duration of the animation, in milliseconds
 * \param[in] restart If true, the animation will restart
 * \param[in, out] strip The led strip to control
 * \return True if the animation is finished
 */
bool double_side_fill(const Color& color, const uint32_t duration,
                      const bool restart, LedStrip& strip);

/**
 * \brief Do police light animation
 * \param[in] duration The duration of the animation, in milliseconds
 * \param[in] restart If true, the animation will restart
 * \param[in, out] strip The led strip to control
 * \return True if the animation is finished
 */
bool police(const uint32_t duration, const bool restart, LedStrip& strip);

/**
 * \brief Do a fade out of the colors currently displayed
 * \param[in] duration The duration of the animation, in milliseconds
 * \param[in] restart If true, the animation will restart
 * \param[in, out] strip The led strip to control
 * \return True if the animation is finished
 */
bool fade_out(const uint32_t duration, const bool restart, LedStrip& strip);

/**
 * \brief Do a fade in of a color
 * \param[in] color class that returns a color to display
 * \param[in] duration The duration of the animation, in milliseconds
 * \param[in] restart If true, the animation will restart
 * \param[in, out] strip The led strip to contro
 * \param[in] firstCutOff between 0 and 1, how much this color will fill the
 * display before suddently cutting of \param[in] secondCutOff between 0 and 1,
 * how much this color will fill the display before suddently cutting of \return
 * True if the animation is finished
 */
bool fade_in(const Color& color, const uint32_t duration, const bool restart,
             LedStrip& strip, const float firstCutOff = 0.0,
             const float secondCutOff = 1.0);

/**
 * Fire animation
 * https://editor.soulmatelights.com/gallery/234-fire
 */
void fire(const uint8_t scalex, const uint8_t scaley, const uint8_t speed,
          const palette_t& palette, LedStrip& strip);

void random_noise(const palette_t& palette, LedStrip& strip, const bool restart,
                  const bool isColorLoop, const uint16_t scale);

void candle(const palette_t& palette, LedStrip& strip);

/**
 * \brief Display some sinewave of colors, going back and forth
 * \param[in] moder: add some random noise
 */
void phases(const bool moder, const uint8_t speed, const palette_t& palette,
            LedStrip& strip);
void mode_2DPolarLights(const uint8_t scale, const uint8_t speed,
                        const palette_t& palette, const bool reset,
                        LedStrip& strip);
void mode_2DDrift(const uint8_t intensity, const uint8_t speed,
                  const palette_t& palette, LedStrip& strip);
void hiphotic(const uint8_t speed, LedStrip& strip);

void mode_2Ddistortionwaves(const uint8_t scale, const uint8_t speed,
                            LedStrip& strip);

void mode_lake(const uint8_t speed, const palette_t& palette, LedStrip& strip);

// Adjustable sinewave. By Andrew Tuline
void mode_sinewave(const uint8_t speed, const uint8_t intensity,
                   const palette_t& palette, LedStrip& strip);

void running_base(bool saw, bool dual, const uint8_t speed,
                  const uint8_t intensity, const palette_t& palette,
                  LedStrip& strip);

};  // namespace animations

#endif

#endif

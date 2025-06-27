#ifdef LMBD_LAMP_TYPE__INDEXABLE

#ifndef TEXT_H
#define TEXT_H

#include <sys/types.h>

#include <cstdint>
#include <string>

#include "src/system/colors/colors.h"
#include "src/system/utils/strip.h"

namespace text {

/**
 * \brief Display a text
 * \param[in] color
 * \param[in] text Text to display
 * \param[in] startXIndex width at which the text starts, in lamp coordinates
 * \param[in] startYIndex height at which the text starts, in lamp coordinates
 * \param[in] scale Between 0 and 1, size of the characters
 * \param[in] reset Set to true to reset the animation
 * \param[in] paddEnd if true, will not consider the animation over until the last letter disappears from the strip
 * \return true if the text was display completly
 */
bool display_text(const Color& color,
                  const std::string& text,
                  const int16_t startXIndex,
                  const int16_t startYIndex,
                  float scale,
                  const bool paddEnd,
                  LedStrip& strip);

/**
 * \brief Display a text going around the lampe
 * \param[in] color
 * \param[in] text Text to display (will start at index X 0)
 * \param[in] startYIndex height at which the text starts, in lamp coordinates
 * \param[in] scale Between 0 and 1, size of the characters
 * \param[in] duration duration of the scroll (in milliseconds)
 * \param[in] reset Set to true to reset the animation
 * \param[in] paddEnd if true, will not consider the animation over until the last letter disappears from the strip
 * \param[in] fadeOut Fade amount per call. low will leave traces, high will be sharp
 * \param[in, out] strip
 * \return true at the end of the animation
 */
bool display_scrolling_text(const Color& color,
                            const std::string& text,
                            const int16_t startYIndex,
                            float scale,
                            const uint32_t duration,
                            const bool reset,
                            const bool paddEnd,
                            const uint8_t fadeOut,
                            LedStrip& strip);

} // namespace text

#endif

#endif
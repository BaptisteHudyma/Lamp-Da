#ifndef UTILS_H
#define UTILS_H

#include "strip.h"
#include "constants.h"
#include <cstdint>


#define SQR(x) ((x)*(x))
#define POW2(x) SQR(x)
#define POW3(x) ((x)*(x)*(x))
#define POW4(x) (POW2(x)*POW2(x))
#define POW7(x) (POW3(x)*POW3(x)*(x))
#define DegToRad(x) ((x)*M_PI/180)
#define FADE16(x) scale16(x,x)
#define FADE8(x) scale8(x,x)

namespace utils {

/**
 * \brief Compute a random color
 */
uint32_t get_random_color();

/**
 * \brief Compute the complementary color of the given color
 * \param[in] color The color to find a complement for
 * \return the complementary color
 */
uint32_t get_complementary_color(const uint32_t color);

/**
 * \brief Compute the complementary color of the given color, with a random variation
 * \param[in] color The color to find a complement for
 * \param[in] tolerance between 0 and 1, the variation tolerance. 1 will give a totally random color, 0 will return the base complementary color
 * \return the random complementary color
 */
uint32_t get_random_complementary_color(const uint32_t color, const float tolerance);

/**
 * \brief Return the color gradient between colorStart to colorEnd
 * \param[in] colorStart Start color of the gradient
 * \param[in] colorEnd End color of the gradient
 * \param[in] level between 0 and 1, the gradient between the two colors
 */
uint32_t get_gradient(const uint32_t colorStart, const uint32_t colorEnd, const float level);

uint32_t hue_to_rgb_sinus(const uint16_t angle);

float map(float x, float in_min, float in_max, float out_min, float out_max);

};

#endif
#ifndef UTILS_H
#define UTILS_H

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

/**
 * \brief Use this to convert color to bytes
 */
union COLOR
{
    uint32_t color;
    
    struct
    {
        uint8_t blue;
        uint8_t green;
        uint8_t red;
        uint8_t white;
    };
};

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
COLOR color_blend(COLOR color1, COLOR color2, uint16_t blend, bool b16 = false);

COLOR color_fade(COLOR c1, uint8_t amount, bool video=false);
COLOR color_add(COLOR c1, COLOR c2, bool fast=false);

uint32_t hue_to_rgb_sinus(const uint16_t angle);

float map(float x, float in_min, float in_max, float out_min, float out_max);

void calcGammaTable(float gamma);
COLOR gamma32(COLOR color);
uint8_t gamma8(uint8_t value);

};

#endif
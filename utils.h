#ifndef UTILS_H
#define UTILS_H

#include "constants.h"

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
 * \brief Compute the hue value of RGB to HSV
 * \param[in] r red channel between 0 and 1
 * \param[in] g green channel between 0 and 1
 * \param[in] b blue channel between 0 and 1
 * \return The value of the hue corresponding to this color
 */
uint16_t rgb2hue(const float r, const float g, const float b);


#endif
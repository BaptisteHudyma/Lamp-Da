#include "utils.h"

#include <Adafruit_NeoPixel.h>

#include <stdlib.h>
#include <math.h>

namespace utils {

uint32_t get_random_color()
{
    return (rand()%255) << 16 | (rand()%255) << 8 | (rand()%255);
}

uint32_t get_complementary_color(const uint32_t color)
{
    const float red = ((color >> 16) & 255)/255.0;
    const float green = ((color >> 8) & 255)/255.0;
    const float blue = ((color >> 0) & 255)/255.0;

    const uint32_t hue = rgb2hue(red, green, blue);

    // add a cardan shift to the hue, to opbtain the symetrical color
    return Adafruit_NeoPixel::gamma32(Adafruit_NeoPixel::ColorHSV(hue + UINT16_MAX/2, 255, 255));
}

uint32_t get_random_complementary_color(const uint32_t color, const float tolerance)
{
    const float red = ((color >> 16) & 255)/255.0;
    const float green = ((color >> 8) & 255)/255.0;
    const float blue = ((color >> 0) & 255)/255.0;

    const uint32_t hue = rgb2hue(red, green, blue);

    // add random offset
    const uint16_t complementaryHue = hue + UINT16_MAX/2.0;
    const float comp = (float)rand()/(float)(RAND_MAX/2) - 1.0; // -1 to 1
    const float offset = comp * INT16_MAX * tolerance;
    return Adafruit_NeoPixel::gamma32(Adafruit_NeoPixel::ColorHSV(complementaryHue + offset, 255, 255));
}


uint32_t get_gradient(const uint32_t colorStart, const uint32_t colorEnd, const float level)
{
    const uint8_t colorStartRed = (colorStart >> 16) & 255;
    const uint8_t colorStartGreen = (colorStart >> 8) & 255;
    const uint8_t colorStartBlue = (colorStart >> 0) & 255;

    const uint8_t colorEndRed = (colorEnd >> 16) & 255;
    const uint8_t colorEndGreen = (colorEnd >> 8) & 255;
    const uint8_t colorEndBlue = (colorEnd >> 0) & 255;

    return Adafruit_NeoPixel::Color(
      colorStartRed + level * (colorEndRed - colorStartRed),
      colorStartGreen + level * (colorEndGreen - colorStartGreen),
      colorStartBlue + level * (colorEndBlue - colorStartBlue)
    );
}


uint16_t rgb2hue(const uint32_t color)
{
    const float r = ((color >> 16) & 255)/255.0;
    const float g = ((color >> 8) & 255)/255.0;
    const float b = ((color >> 0) & 255)/255.0;

    return rgb2hue(r, g, b);
}

uint16_t rgb2hue(const float r, const float g, const float b)
{
    const float cmax = fmax(r, fmax(g, b)); // maximum of r, g, b
    const float cmin = fmin(r, fmin(g, b)); // minimum of r, g, b
    const float diff = cmax - cmin; // diff of cmax and cmin.
  
    // if cmax and cmax are equal then h = 0
    if (cmax == cmin)
        return 0;
    // if cmax equal r then compute h
    else if (cmax == r)
        return fmod(60.0 * ((g - b) / diff) + 360, 360.0) / 360.0 * UINT16_MAX;
    // if cmax equal g then compute h
    else if (cmax == g)
        return fmod(60.0 * ((b - r) / diff) + 120, 360.0) / 360.0 * UINT16_MAX;
    // if cmax equal b then compute h
    else if (cmax == b)
        return fmod(60.0 * ((r - g) / diff) + 240, 360.0) / 360.0 * UINT16_MAX;

  return 0;
}

};
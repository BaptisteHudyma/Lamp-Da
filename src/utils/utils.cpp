#include "utils.h"

#include <Adafruit_NeoPixel.h>
#include "colorspace.h"

#include <cstdint>
#include <stdlib.h>
#include <math.h>

namespace utils {

uint32_t get_random_color()
{
    static uint8_t count;
    
    uint8_t color[3];
    color[count] = random(256);
    uint8_t a0 = random(1);
    uint8_t a1 = ((!a0) + count + 1) % 3;
    a0 = (count + a0 + 1) % 3;
    color[a0] = 255 - color[count];
    color[a1] = 0;
    count += random(15); // to avoid repeating patterns
    count %= 3;

    union COLOR colorArray;
    colorArray.red = color[0];
    colorArray.green = color[1];
    colorArray.blue = color[2];
    return colorArray.color;
}

uint32_t get_complementary_color(const uint32_t color)
{
    COLOR c;
    c.color = color;
    const uint16_t hue = ColorSpace::HSV(c).get_scaled_hue();

    const uint16_t finalHue = (uint32_t)(hue + UINT16_MAX/2.0) % UINT16_MAX;
    // add a cardan shift to the hue, to opbtain the symetrical color
    return utils::hue2rgbSinus(map(finalHue, 0, UINT16_MAX, 0, 360));
}

uint32_t get_random_complementary_color(const uint32_t color, const float tolerance)
{
    COLOR c;
    c.color = color;
    const uint16_t hue = ColorSpace::HSV(c).get_scaled_hue();

    // add random offset
    const float comp = 0.1 + (float)rand()/(float)RAND_MAX;   // 0.1 to 1.1
    const uint16_t finalHue = fmod(hue + UINT16_MAX / 2 + comp * tolerance * UINT16_MAX, 360.0f);   // add offset to the hue
    return utils::hue2rgbSinus(finalHue);
}

uint32_t get_gradient(const uint32_t colorStart, const uint32_t colorEnd, const float level)
{
    union COLOR colorStartArray, colorEndArray;
    colorStartArray.color= colorStart;
    colorEndArray.color = colorEnd;
    
    return Adafruit_NeoPixel::Color(
      colorStartArray.red + level * (colorEndArray.red - colorStartArray.red),
      colorStartArray.green + level * (colorEndArray.green - colorStartArray.green),
      colorStartArray.blue + level * (colorEndArray.blue - colorStartArray.blue)
    );
}

uint32_t hue2rgbSinus(const uint16_t angle)
{
    static const uint8_t lights[360] = {
    0,   0,   0,   0,   0,   1,   1,   2, 
    2,   3,   4,   5,   6,   7,   8,   9, 
    11,  12,  13,  15,  17,  18,  20,  22, 
    24,  26,  28,  30,  32,  35,  37,  39, 
    42,  44,  47,  49,  52,  55,  58,  60, 
    63,  66,  69,  72,  75,  78,  81,  85, 
    88,  91,  94,  97, 101, 104, 107, 111, 
    114, 117, 121, 124, 127, 131, 134, 137, 
    141, 144, 147, 150, 154, 157, 160, 163, 
    167, 170, 173, 176, 179, 182, 185, 188, 
    191, 194, 197, 200, 202, 205, 208, 210, 
    213, 215, 217, 220, 222, 224, 226, 229, 
    231, 232, 234, 236, 238, 239, 241, 242, 
    244, 245, 246, 248, 249, 250, 251, 251, 
    252, 253, 253, 254, 254, 255, 255, 255, 
    255, 255, 255, 255, 254, 254, 253, 253, 
    252, 251, 251, 250, 249, 248, 246, 245, 
    244, 242, 241, 239, 238, 236, 234, 232, 
    231, 229, 226, 224, 222, 220, 217, 215, 
    213, 210, 208, 205, 202, 200, 197, 194, 
    191, 188, 185, 182, 179, 176, 173, 170, 
    167, 163, 160, 157, 154, 150, 147, 144, 
    141, 137, 134, 131, 127, 124, 121, 117, 
    114, 111, 107, 104, 101,  97,  94,  91, 
    88,  85,  81,  78,  75,  72,  69,  66, 
    63,  60,  58,  55,  52,  49,  47,  44, 
    42,  39,  37,  35,  32,  30,  28,  26, 
    24,  22,  20,  18,  17,  15,  13,  12, 
    11,   9,   8,   7,   6,   5,   4,   3, 
    2,   2,   1,   1,   0,   0,   0,   0, 
    0,   0,   0,   0,   0,   0,   0,   0, 
    0,   0,   0,   0,   0,   0,   0,   0, 
    0,   0,   0,   0,   0,   0,   0,   0, 
    0,   0,   0,   0,   0,   0,   0,   0, 
    0,   0,   0,   0,   0,   0,   0,   0, 
    0,   0,   0,   0,   0,   0,   0,   0, 
    0,   0,   0,   0,   0,   0,   0,   0, 
    0,   0,   0,   0,   0,   0,   0,   0, 
    0,   0,   0,   0,   0,   0,   0,   0, 
    0,   0,   0,   0,   0,   0,   0,   0, 
    0,   0,   0,   0,   0,   0,   0,   0, 
    0,   0,   0,   0,   0,   0,   0,   0, 
    0,   0,   0,   0,   0,   0,   0,   0, 
    0,   0,   0,   0,   0,   0,   0,   0, 
    0,   0,   0,   0,   0,   0,   0,   0
    };

    union COLOR colorArray;
    colorArray.red = lights[(angle + 120) % 360];
    colorArray.green = lights[angle];
    colorArray.blue = lights[(angle + 240) % 360];
    return colorArray.color;
}

};
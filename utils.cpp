#include "utils.h"

#include <Adafruit_NeoPixel.h>

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

    return color[0] << 16 | color[1] << 8 | color[2];
}

uint32_t get_complementary_color(const uint32_t color)
{
    const uint16_t hue = rgb2hue(color);
    const uint16_t finalHue = (uint32_t)(hue + UINT16_MAX/2.0) % UINT16_MAX;
    // add a cardan shift to the hue, to opbtain the symetrical color
    return utils::hue2rgbSinus(map(finalHue, 0, UINT16_MAX, 0, 360));
}

uint32_t get_random_complementary_color(const uint32_t color, const float tolerance)
{
    const uint16_t hue = rgb2hue(color);

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

uint16_t rgb2hue(const uint32_t color)
{
    union COLOR colorArray;
    colorArray.color = color;
    return rgb2hue(colorArray.red/255.0, colorArray.green/255.0, colorArray.blue/255.0);
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

uint32_t hue2rgb(const uint16_t angle)
{
    static const uint8_t HSVlights[61] = {
        0, 4, 8, 13, 17, 21, 25, 30, 34, 38, 42, 47, 51, 55, 59, 64, 68, 72, 76,
        81, 85, 89, 93, 98, 102, 106, 110, 115, 119, 123, 127, 132, 136, 140, 144,
        149, 153, 157, 161, 166, 170, 174, 178, 183, 187, 191, 195, 200, 204, 208,
        212, 217, 221, 225, 229, 234, 238, 242, 246, 251, 255
    };

    uint8_t red, green, blue;
    if (angle<60) {red = 255; green = HSVlights[angle]; blue = 0;}
    else if (angle<120) {red = HSVlights[120-angle]; green = 255; blue = 0;}
    else if (angle<180) {red = 0, green = 255; blue = HSVlights[angle-120];}
    else if (angle<240) {red = 0, green = HSVlights[240-angle]; blue = 255;}
    else if (angle<300) {red = HSVlights[angle-240], green = 0; blue = 255;}
    else {red = 255, green = 0; blue = HSVlights[360-angle];} 
    
    return red << 16 | green << 8 | blue;
}

uint32_t hue2rgbPower(const uint16_t angle)
{
    static const uint8_t HSVpower[121] = {
        0, 2, 4, 6, 8, 11, 13, 15, 17, 19, 21, 23, 25, 28, 30, 32, 34, 36, 38, 40,
        42, 45, 47, 49, 51, 53, 55, 57, 59, 62, 64, 66, 68, 70, 72, 74, 76, 79, 81, 
        83, 85, 87, 89, 91, 93, 96, 98, 100, 102, 104, 106, 108, 110, 113, 115, 117, 
        119, 121, 123, 125, 127, 130, 132, 134, 136, 138, 140, 142, 144, 147, 149, 
        151, 153, 155, 157, 159, 161, 164, 166, 168, 170, 172, 174, 176, 178, 181, 
        183, 185, 187, 189, 191, 193, 195, 198, 200, 202, 204, 206, 208, 210, 212, 
        215, 217, 219, 221, 223, 225, 227, 229, 232, 234, 236, 238, 240, 242, 244, 
        246, 249, 251, 253, 255
    };

    uint8_t red, green, blue;
    if (angle<120) {red = HSVpower[120-angle]; green = HSVpower[angle]; blue = 0;}
    else if (angle<240) {red = 0;  green = HSVpower[240-angle]; blue = HSVpower[angle-120];}
    else {red = HSVpower[angle-240]; green = 0; blue = HSVpower[360-angle];}

    return red << 16 | green << 8 | blue;
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

    return (lights[(angle + 120) % 360] << 16) | (lights[angle]) << 8 | (lights[(angle + 240) % 360]);
}

};
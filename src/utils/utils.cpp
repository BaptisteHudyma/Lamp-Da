#include "utils.h"

#include <Adafruit_NeoPixel.h>

#include <cstdint>
#include <stdlib.h>
#include <math.h>

namespace utils {

namespace ColorSpace {

COLOR XYZ::get_rgb() const
{
    double x = this->x / 100.0;
    double y = this->y / 100.0;
    double z = this->z / 100.0;

    double r = x * 3.2404542 + y * -1.5371385 + z * -0.4985314;
    double g = x * -0.9692660 + y * 1.8760108 + z * 0.0415560;
    double b = x * 0.0556434 + y * -0.2040259 + z * 1.0572252;

    r = ((r > 0.0031308) ? (1.055*std::pow(r, 1 / 2.4) - 0.055) : (12.92*r)) * 255.0;
    g = ((g > 0.0031308) ? (1.055*std::pow(g, 1 / 2.4) - 0.055) : (12.92*g)) * 255.0;
    b = ((b > 0.0031308) ? (1.055*std::pow(b, 1 / 2.4) - 0.055) : (12.92*b)) * 255.0;
    
    COLOR c;
    c.red = r;
    c.green = g;
    c.blue = b;
    return c;
}

void XYZ::from_rgb(const COLOR& rgb)
{
    double r = rgb.red / 255.0;
    double g = rgb.green / 255.0;
    double b = rgb.blue / 255.0;

    r = ((r > 0.04045) ? std::pow((r + 0.055) / 1.055, 2.4) : (r / 12.92)) * 100.0;
    g = ((g > 0.04045) ? std::pow((g + 0.055) / 1.055, 2.4) : (g / 12.92)) * 100.0;
    b = ((b > 0.04045) ? std::pow((b + 0.055) / 1.055, 2.4) : (b / 12.92)) * 100.0;

    this->x = r*0.4124564 + g*0.3575761 + b*0.1804375;
    this->y = r*0.2126729 + g*0.7151522 + b*0.0721750;
    this->z = r*0.0193339 + g*0.1191920 + b*0.9503041;
}

COLOR HSV::get_rgb() const
{
    int range = (int)std::floor(this->h / 60.0);
    double c = this->v*this->s;
    double x = c*(1 - abs(std::fmod(this->h / 60.0, 2) - 1.0));
    double m = this->v - c;

    COLOR out;
    switch (range) {
        case 0:
            out.red = (c + m) * 255;
            out.green = (x + m) * 255;
            out.blue = m * 255;
            break;
        case 1:
            out.red = (x + m) * 255;
            out.green = (c + m) * 255;
            out.blue = m * 255;
            break;
        case 2:
            out.red = m * 255;
            out.green = (c + m) * 255;
            out.blue = (x + m) * 255;
            break;
        case 3:
            out.red = m * 255;
            out.green = (x + m) * 255;
            out.blue = (c + m) * 255;
            break;
        case 4:
            out.red = (x + m) * 255;
            out.green = m * 255;
            out.blue = (c + m) * 255;
            break;
        default:		// case 5:
            out.red = (c + m) * 255;
            out.green = m * 255;
            out.blue = (x + m) * 255;
            break;
    }
    return out;
}

void HSV::from_rgb(const COLOR& rgb)
{
    double r = rgb.red / 255.0;
    double g = rgb.green / 255.0;
    double b = rgb.blue / 255.0;

    double min = std::fmin(r, std::fmin(g, b));
    double max = std::fmax(r, std::fmax(g, b));
    double delta = max - min;

    this->v = max;
    this->s = (max > 1e-3) ? (delta / max) : 0;

    if (delta == 0) {
        this->h = 0;
    }
    else {
        if (r == max) {
            this->h = (g - b) / delta;
        }
        else if (g == max) {
            this->h = 2 + (b - r) / delta;
        }
        else if (b == max) {
            this->h = 4 + (r - g) / delta;
        }

        this->h *= 60;
        this->h = std::fmod(this->h + 360, 360);
    }
}

// get the rgb form (for display)
COLOR LAB::get_rgb() const
{
    const XYZ &white = XYZ::get_white();
    
    double y = (this->l + 16.0) / 116.0;
    double x = this->a / 500.0 + y;
    double z = y - this->b / 200.0;

    double x3 = POW3(x);
    double y3 = POW3(y);
    double z3 = POW3(z);

    x = ((x3 > 0.008856) ? x3 : ((x - 16.0 / 116.0) / 7.787)) * white.x;
    y = ((y3 > 0.008856) ? y3 : ((y - 16.0 / 116.0) / 7.787)) * white.y;
    z = ((z3 > 0.008856) ? z3 : ((z - 16.0 / 116.0) / 7.787)) * white.z;

    XYZ xyz(x, y, z);
    return xyz.get_rgb();
}

void LAB::from_rgb(const COLOR& rgb)
{
    const XYZ &white = XYZ::get_white();
    
    XYZ xyz(rgb);

    double x = xyz.x / white.x;
    double y = xyz.y / white.y;
    double z = xyz.z / white.z;

    x = (x > 0.008856) ? std::cbrt(x) : (7.787 * x + 16.0 / 116.0);
    y = (y > 0.008856) ? std::cbrt(y) : (7.787 * y + 16.0 / 116.0);
    z = (z > 0.008856) ? std::cbrt(z) : (7.787 * z + 16.0 / 116.0);

    this->l = (116.0 * y) - 16;
    this->a = 500 * (x - y);
    this->b = 200 * (y - z);
}

COLOR LCH::get_rgb() const
{
    const double newH = this->h * M_PI / 180;

    LAB lab(this->l, std::cos(newH) * this->c, std::sin(newH) * this->c);
    return lab.get_rgb();
}

void LCH::from_rgb(const COLOR& rgb)
{
    LAB lab(rgb);
    
    double l = lab.l;
    double c = std::sqrt(lab.a*lab.a + lab.b*lab.b);
    double h = std::atan2(lab.b, lab.a);

    h = h / M_PI * 180;
    if (h < 0) {
        h += 360;
    }
    else if (h >= 360) {
        h -= 360;
    }

    this->l = l;
    this->c = c;
    this->h = h;
}

// get the rgb form (for display)
COLOR OKLAB::get_rgb() const
{
    double l = this->l + 0.3963377774 * this->a + 0.2158037573 * this->b;
    double m = this->l - 0.1055613458 * this->a - 0.0638541728 * this->b;
    double s = this->l - 0.0894841775 * this->a - 1.2914855480 * this->b;
    
    l = l * l * l;
    m = m * m * m;
    s = s * s * s;
    
    double r =  4.0767245293 * l - 3.3072168827 * m + 0.2307590544 * s;
    double g = -1.2681437731 * l + 2.6093323231 * m - 0.3411344290 * s;
    double b = -0.0041119885 * l - 0.7034763098 * m + 1.7068625689 * s;
    
    COLOR out;
    out.red = ((r > 0.0031308) ? (1.055*std::pow(r, 1 / 2.4) - 0.055) : (12.92*r)) * 255.0;
    out.green = ((g > 0.0031308) ? (1.055*std::pow(g, 1 / 2.4) - 0.055) : (12.92*g)) * 255.0;
    out.blue = ((b > 0.0031308) ? (1.055*std::pow(b, 1 / 2.4) - 0.055) : (12.92*b)) * 255.0;
    return out;
}

void OKLAB::from_rgb(const COLOR& rgb)
{
    double r = rgb.red / 255.0;
    double g = rgb.green / 255.0;
    double b = rgb.blue / 255.0;
    
    r = ((r > 0.04045) ? std::pow((r + 0.055) / 1.055, 2.4) : (r / 12.92));
    g = ((g > 0.04045) ? std::pow((g + 0.055) / 1.055, 2.4) : (g / 12.92));
    b = ((b > 0.04045) ? std::pow((b + 0.055) / 1.055, 2.4) : (b / 12.92));
    
    double l = 0.4122214708f * r + 0.5363325363f * g + 0.0514459929f * b;
    double m = 0.2119034982f * r + 0.6806995451f * g + 0.1073969566f * b;
    double s = 0.0883024619f * r + 0.2817188376f * g + 0.6299787005f * b;
    
    
    l = std::cbrt(l);
    m = std::cbrt(m);
    s = std::cbrt(s);
    
    this->l = 0.2104542553f * l + 0.7936177850f * m - 0.0040720468f * s;
    this->a = 1.9779984951f * l - 2.4285922050f * m + 0.4505937099f * s;
    this->b = 0.0259040371f * l + 0.7827717662f * m - 0.8086757660f * s;
}

COLOR OKLCH::get_rgb() const
{
    const double newH = this->h * M_PI / 180;

    OKLAB lab(this->l, std::cos(newH) * this->c, std::sin(newH) * this->c);
    return lab.get_rgb();
}

void OKLCH::from_rgb(const COLOR& rgb)
{
    OKLAB lab(rgb);

    double l = lab.l;
    double c = std::sqrt(lab.a*lab.a + lab.b*lab.b);
    double h = std::atan2(lab.b, lab.a);
    
    h = h / M_PI * 180;
    if (h < 0) {
        h += 360;
    }
    else if (h >= 360) {
        h -= 360;
    }
    
    this->l = l;
    this->c = c;
    this->h = h;
}

}

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
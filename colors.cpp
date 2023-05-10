#include "colors.h"

#include <Adafruit_NeoPixel.h>
#include "constants.h"
#include "utils.h"


uint32_t GenerateSolidColor::get_color(const uint16_t index, const uint16_t maxIndex) const
{
    return _color;
}

uint32_t GenerateRainbowColor::get_color(const uint16_t index, const uint16_t maxIndex) const
{
    const uint16_t hue = map(index, 0, maxIndex, 0, UINT16_MAX);
    return Adafruit_NeoPixel::gamma32(Adafruit_NeoPixel::ColorHSV(hue));
}

uint32_t GenerateGradientColor::get_color(const uint16_t index, const uint16_t maxIndex) const
{
    return utils::get_gradient(_colorStart, _colorEnd, index / (float)maxIndex);
}

uint32_t GenerateRainbowSwirl::get_color(const uint16_t index, const uint16_t maxIndex) const
{
    uint32_t pixelHue = _firstPixelHue + (index * UINT16_MAX / maxIndex);
    return Adafruit_NeoPixel::gamma32(Adafruit_NeoPixel::ColorHSV(pixelHue));
}

uint32_t GenerateRainbowPulse::get_color(const uint16_t index, const uint16_t maxIndex) const
{
    return Adafruit_NeoPixel::gamma32(Adafruit_NeoPixel::ColorHSV(_currentPixelHue));
}

GenerateRandomColor::GenerateRandomColor()
: _color(utils::get_random_color())
{
}

void GenerateRandomColor::internal_update(const uint32_t deltaTimeMilli) 
{
    _color = utils::get_random_color();
}


void GenerateComplementaryColor::internal_update(const uint32_t deltaTimeMilli)
{
    _color = utils::get_random_complementary_color(_color, _randomVariation);
};
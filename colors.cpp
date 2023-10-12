#include "colors.h"

#include <Adafruit_NeoPixel.h>
#include <cmath>
#include <cstdint>
#include "constants.h"
#include "palettes.h"
#include "utils.h"


uint32_t GenerateSolidColor::get_color(const uint16_t index, const uint16_t maxIndex) const
{
    return _color;
}

uint32_t GenerateRainbowColor::get_color(const uint16_t index, const uint16_t maxIndex) const
{
    const uint16_t hue = map(index, 0, maxIndex, 0, 360);
    return utils::hue2rgbSinus(hue);
}

uint32_t GenerateGradientColor::get_color(const uint16_t index, const uint16_t maxIndex) const
{
    return utils::get_gradient(_colorStart, _colorEnd, index / (float)maxIndex);
}

uint32_t GenerateRoundColor::get_color(const uint16_t index, const uint16_t maxIndex) const
{
    const double segmentsPerTurns = 3.1;
    const double modulo = std::fmod(index, segmentsPerTurns) / segmentsPerTurns;
    return utils::hue2rgbSinus(modulo * 360.0);
}

uint32_t GenerateRainbowSwirl::get_color(const uint16_t index, const uint16_t maxIndex) const
{
    const uint16_t pixelHue = _firstPixelHue + (index * UINT16_MAX / maxIndex);
    return utils::hue2rgbSinus(map(pixelHue, 0, UINT16_MAX, 0, 360));
}

uint32_t GeneratePaletteStep::get_color(const uint16_t index, const uint16_t maxIndex) const
{
    return get_color_from_palette(_index, *_paletteRef);
}

uint32_t GeneratePaletteIndexed::get_color(const uint16_t index, const uint16_t maxIndex) const
{
    return get_color_from_palette(_index, *_paletteRef);
}

uint32_t GenerateRainbowPulse::get_color(const uint16_t index, const uint16_t maxIndex) const
{
    const double hue = double(_currentPixelHue) / double(UINT16_MAX) * 360.0;

    auto res = utils::ColorSpace::OKLCH(0.752, 0.126, hue);
    return res.get_rgb().color;
}


uint32_t GenerateRainbowIndex::get_color(const uint16_t index, const uint16_t maxIndex) const
{
    return Adafruit_NeoPixel::ColorHSV(_currentPixelHue);
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
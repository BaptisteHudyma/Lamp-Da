#ifdef LMBD_LAMP_TYPE__INDEXABLE

// this file is active only if LMBD_LAMP_TYPE=indexable
#include "colors.h"

#include <cmath>
#include <cstdint>

#include "src/system/colors/palettes.h"
#include "src/system/utils/colorspace.h"
#include "src/system/utils/strip.h"
#include "src/system/utils/utils.h"

uint32_t GenerateSolidColor::get_color_internal(const uint16_t index, const uint16_t maxIndex) const { return _color; }

uint32_t GenerateRainbowColor::get_color_internal(const uint16_t index, const uint16_t maxIndex) const
{
  const uint16_t hue = lmpd_map<uint16_t>(lmpd_constrain<uint16_t>(index, 0, maxIndex), 0, maxIndex, 0, 360);
  return utils::hue_to_rgb_sinus(hue);
}

uint32_t GenerateGradientColor::get_color_internal(const uint16_t index, const uint16_t maxIndex) const
{
  return utils::get_gradient(_colorStart, _colorEnd, index / (float)maxIndex);
}

uint32_t GenerateRoundColor::get_color_internal(const uint16_t index, const uint16_t maxIndex) const
{
  const double segmentsPerTurns = 3.1;
  const double modulo = std::fmod(index, segmentsPerTurns) / segmentsPerTurns;
  return utils::hue_to_rgb_sinus(modulo * 360.0);
}

uint32_t GenerateRainbowSwirl::get_color_internal(const uint16_t index, const uint16_t maxIndex) const
{
  const uint16_t pixelHue = _firstPixelHue + (index * UINT16_MAX / maxIndex);
  return utils::hue_to_rgb_sinus(lmpd_map<uint16_t>(pixelHue, 0, UINT16_MAX, 0, 360));
}

uint32_t GeneratePalette::get_color_internal(const uint16_t index, const uint16_t maxIndex) const
{
  const uint16_t indexAdded = (index + _index) % UINT16_MAX;
  return get_color_from_palette(static_cast<uint8_t>(indexAdded / static_cast<float>(maxIndex) * UINT8_MAX),
                                *_paletteRef);
}

uint32_t GeneratePaletteStep::get_color_internal(const uint16_t index, const uint16_t maxIndex) const
{
  return get_color_from_palette(_index, *_paletteRef);
}

uint32_t GeneratePaletteIndexed::get_color_internal(const uint16_t index, const uint16_t maxIndex) const
{
  return get_color_from_palette(_index, *_paletteRef);
}

uint32_t GenerateRainbowPulse::get_color_internal(const uint16_t index, const uint16_t maxIndex) const
{
  return LedStrip::ColorHSV(_currentPixelHue);
}

uint32_t GenerateRainbowIndex::get_color_internal(const uint16_t index, const uint16_t maxIndex) const
{
  return LedStrip::ColorHSV(_currentPixelHue);
}

uint32_t GeneratePastelPulse::get_color_internal(const uint16_t index, const uint16_t maxIndex) const
{
  const double hue = double(_currentPixelHue) / double(UINT16_MAX) * 360.0;

  // using OKLCH will create softer colors
  auto res = utils::ColorSpace::OKLCH(0.752, 0.126, hue);
  return res.get_rgb().color;
}

GenerateRandomColor::GenerateRandomColor() : _color(utils::get_random_color()) {}

void GenerateRandomColor::internal_update(const uint32_t deltaTimeMilli) { _color = utils::get_random_color(); }

void GenerateComplementaryColor::internal_update(const uint32_t deltaTimeMilli)
{
  _color = utils::get_random_complementary_color(_color, _randomVariation);
};

#endif

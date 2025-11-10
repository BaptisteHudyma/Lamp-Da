#ifdef LMBD_LAMP_TYPE__INDEXABLE

// this file is active only if LMBD_LAMP_TYPE=indexable
#include "palettes.h"

#include <cstdint>

#include "src/system/utils/utils.h"

/// Cloudy color palette/ blue to blue-white
const palette_t PaletteCloudColors = {Blue,
                                      DarkBlue,
                                      DarkBlue,
                                      DarkBlue,
                                      DarkBlue,
                                      DarkBlue,
                                      DarkBlue,
                                      DarkBlue,
                                      Blue,
                                      DarkBlue,
                                      SkyBlue,
                                      SkyBlue,
                                      LightBlue,
                                      White,
                                      LightBlue,
                                      SkyBlue};

/// Lava color palette
const palette_t PaletteLavaColors = {Black,
                                     Maroon,
                                     Black,
                                     Maroon,
                                     DarkRed,
                                     Maroon,
                                     DarkRed,
                                     DarkRed,
                                     DarkRed,
                                     Red,
                                     Orange,
                                     White,
                                     Orange,
                                     Red,
                                     DarkRed,
                                     Black};

/// Fire color palette
const palette_t PaletteFlameColors = {Orange,
                                      Orange,
                                      Orange,
                                      Orange,
                                      Orange,
                                      Orange,
                                      Orange,
                                      Orange,
                                      Orange,
                                      Orange,
                                      Orange,
                                      Orange,
                                      Candle,
                                      Orange,
                                      Orange,
                                      Gold};

/// Ocean colors, blues and whites
const palette_t PaletteOceanColors = {MidnightBlue,
                                      DarkBlue,
                                      MidnightBlue,
                                      Navy,
                                      DarkBlue,
                                      MediumBlue,
                                      SeaGreen,
                                      Teal,
                                      CadetBlue,
                                      Blue,
                                      DarkCyan,
                                      CornflowerBlue,
                                      Aquamarine,
                                      SeaGreen,
                                      Aqua,
                                      LightSkyBlue};

/// Water colors, blues
const palette_t PaletteWaterColors = {MidnightBlue,
                                      DarkBlue,
                                      MidnightBlue,
                                      Navy,
                                      DarkBlue,
                                      MediumBlue,
                                      MediumBlue,
                                      Teal,
                                      CadetBlue,
                                      Blue,
                                      DarkCyan,
                                      CornflowerBlue,
                                      Aquamarine,
                                      MediumBlue,
                                      Aqua,
                                      LightSkyBlue};

/// Forest colors, greens
const palette_t PaletteForestColors = {DarkGreen,
                                       DarkGreen,
                                       DarkOliveGreen,
                                       DarkGreen,
                                       Green,
                                       ForestGreen,
                                       OliveDrab,
                                       Green,
                                       SeaGreen,
                                       MediumAquamarine,
                                       LimeGreen,
                                       YellowGreen,
                                       LightGreen,
                                       LawnGreen,
                                       MediumAquamarine,
                                       ForestGreen};

/// HSV Rainbow
const palette_t PaletteRainbowColors = {0xFF0000,
                                        0xD52A00,
                                        0xAB5500,
                                        0xAB7F00,
                                        0xABAB00,
                                        0x56D500,
                                        0x00FF00,
                                        0x00D52A,
                                        0x00AB55,
                                        0x0056AA,
                                        0x0000FF,
                                        0x2A00D5,
                                        0x5500AB,
                                        0x7F0081,
                                        0xAB0055,
                                        0xD5002B};

// basically, HSV with no green. looks better when lighing people
const palette_t PalettePartyColors = {0x5500AB,
                                      0x84007C,
                                      0xB5004B,
                                      0xE5001B,
                                      0xE81700,
                                      0xB84700,
                                      0xAB7700,
                                      0xABAB00,
                                      0xAB5500,
                                      0xDD2200,
                                      0xF2000E,
                                      0xC2003E,
                                      0x8F0071,
                                      0x5F00A1,
                                      0x2F00D0,
                                      0x0007F9};

// Black body radiation
const palette_t PaletteBlackBodyColors = {0xff3800,
                                          0xff5300,
                                          0xff6500,
                                          0xff7300,
                                          0xff7e00,
                                          0xff8912,
                                          0xff932c,
                                          0xff9d3f,
                                          0xffa54f,
                                          0xffad5e,
                                          0xffb46b,
                                          0xffbb78,
                                          0xffc184,
                                          0xffc78f,
                                          0xffcc99,
                                          0xffd1a3};

const palette_t PaletteHeatColors = {0x000000,
                                     0x330000,
                                     0x660000,
                                     0x990000,
                                     0xCC0000,
                                     0xFF0000,
                                     0xFF3300,
                                     0xFF6600,
                                     0xFF9900,
                                     0xFFCC00,
                                     0xFFFF00,
                                     0xFFFF33,
                                     0xFFFF66,
                                     0xFFFF99,
                                     0xFFFFCC};

const palette_t PaletteAuroraColors = {0x000000,
                                       0x003300,
                                       0x006600,
                                       0x009900,
                                       0x00cc00,
                                       0x00ff00,
                                       0x33ff00,
                                       0x66ff00,
                                       0x99ff00,
                                       0xccff00,
                                       0xffff00,
                                       0xffcc00,
                                       0xff9900,
                                       0xff6600,
                                       0xff3300,
                                       0xff0000};

uint32_t get_color_from_palette(const uint8_t index, const palette_t& palette, const uint8_t brightness)
{
  const uint8_t renormIndex = index >> 4;  // convert to [0; 15] (divide by 16)
  const uint8_t blendIndex = index & 0x0F; // mask with 15 (get the less significant part, that will
                                           // be the blend factor)

  const uint32_t entry = palette[renormIndex];

  // convert to rgb
  union COLOR color;
  color.color = entry;

  uint8_t red1 = color.red;
  uint8_t green1 = color.green;
  uint8_t blue1 = color.blue;

  // need to blenc the palette
  if (blendIndex != 0)
  {
    union COLOR nextColor;
    if (renormIndex == 15)
    {
      nextColor.color = palette[0];
    }
    else
    {
      nextColor.color = palette[1 + renormIndex];
    }

    const uint8_t tmpF2 = blendIndex << 4;
    const float f1 = (255 - tmpF2) / 256.0;
    const float f2 = tmpF2 / 256.0;

    const uint8_t red2 = nextColor.red;
    red1 = red1 * f1;
    red1 += red2 * f2;

    const uint8_t green2 = nextColor.green;
    green1 = green1 * f1;
    green1 += green2 * f2;

    const uint8_t blue2 = nextColor.blue;
    blue1 = blue1 * f1;
    blue1 += blue2 * f2;
  }

  if (brightness != 255)
  {
    if (brightness != 0)
    {
      const float adjustedBirghtness = (brightness + 1) / 256.0; // adjust for rounding
      // Now, since brightness is nonzero, we don't need the full scale8_video
      // logic; we can just to scale8 and then add one (unless scale8 fixed) to
      // all nonzero inputs.
      if (red1)
      {
        red1 = red1 * adjustedBirghtness;
        ++red1;
      }
      if (green1)
      {
        green1 = green1 * adjustedBirghtness;
        ++green1;
      }
      if (blue1)
      {
        blue1 = blue1 * adjustedBirghtness;
        ++blue1;
      }
    }
    else
    {
      red1 = 0;
      green1 = 0;
      blue1 = 0;
    }
  }

  // convert back to color
  color.red = red1;
  color.green = green1;
  color.blue = blue1;
  // return color code
  return color.color;
}

uint32_t get_color_from_palette(const uint16_t index, const palette_t& palette, const uint8_t brightness)
{
  const float ramp = (index / (float)UINT16_MAX) * PALETTE_SIZE;

  const uint8_t renormIndex = min<uint8_t>(floorf(ramp), PALETTE_SIZE - 1);     // convert to [0; 15] (divide by 16)
  const float blendIndex = lmpd_constrain<float>(ramp - renormIndex, 0.0, 1.0); // get the fractional part

  const uint32_t entry = palette[renormIndex];

  // convert to rgb
  union COLOR color;
  color.color = entry;

  uint8_t red1 = color.red;
  uint8_t green1 = color.green;
  uint8_t blue1 = color.blue;

  // need to blenc the palette
  if (blendIndex > 0)
  {
    union COLOR nextColor;
    if (renormIndex == PALETTE_SIZE - 1)
    {
      nextColor.color = palette[0];
    }
    else
    {
      nextColor.color = palette[1 + renormIndex];
    }

    const float f1 = blendIndex;
    const float f2 = 1.0 - blendIndex;

    const uint8_t red2 = nextColor.red;
    red1 = red1 * f1;
    red1 += red2 * f2;

    const uint8_t green2 = nextColor.green;
    green1 = green1 * f1;
    green1 += green2 * f2;

    const uint8_t blue2 = nextColor.blue;
    blue1 = blue1 * f1;
    blue1 += blue2 * f2;
  }

  if (brightness != 255)
  {
    if (brightness != 0)
    {
      const float adjustedBirghtness = (brightness + 1) / 256.0; // adjust for rounding
      // Now, since brightness is nonzero, we don't need the full scale8_video
      // logic; we can just to scale8 and then add one (unless scale8 fixed) to
      // all nonzero inputs.
      if (red1)
      {
        red1 = red1 * adjustedBirghtness;
        ++red1;
      }
      if (green1)
      {
        green1 = green1 * adjustedBirghtness;
        ++green1;
      }
      if (blue1)
      {
        blue1 = blue1 * adjustedBirghtness;
        ++blue1;
      }
    }
    else
    {
      red1 = 0;
      green1 = 0;
      blue1 = 0;
    }
  }

  // convert back to color
  color.red = red1;
  color.green = green1;
  color.blue = blue1;
  // return color code
  return color.color;
}

#endif

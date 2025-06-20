#ifndef PALETTES_H
#define PALETTES_H

// this file is active only if LMBD_LAMP_TYPE=indexable
#ifdef LMBD_LAMP_TYPE__INDEXABLE

#include <array>
#include <cstdint>

static constexpr uint8_t PALETTE_SIZE = 16;
using palette_t = std::array<uint32_t, PALETTE_SIZE>;

/// @brief Color temperature values
/// @details These color values are separated into two groups: black body
/// radiators and gaseous light sources.
///
/// Black body radiators emit a (relatively) continuous spectrum,
/// and can be described as having a Kelvin 'temperature'. This includes things
/// like candles, tungsten lightbulbs, and sunlight.
///
/// Gaseous light sources emit discrete spectral bands, and while we can
/// approximate their aggregate hue with RGB values, they don't actually
/// have a proper Kelvin temperature.
///
/// @see https://en.wikipedia.org/wiki/Color_temperature
typedef enum
{
  // Black Body Radiators
  // @{
  /// 1900 Kelvin
  Candle = 0xFF9329 /* 1900 K, 255, 147, 41 */,
  /// 2600 Kelvin
  Tungsten40W = 0xFFC58F /* 2600 K, 255, 197, 143 */,
  /// 2850 Kelvin
  Tungsten100W = 0xFFD6AA /* 2850 K, 255, 214, 170 */,
  /// 3200 Kelvin
  Halogen = 0xFFF1E0 /* 3200 K, 255, 241, 224 */,
  /// 5200 Kelvin
  CarbonArc = 0xFFFAF4 /* 5200 K, 255, 250, 244 */,
  /// 5400 Kelvin
  HighNoonSun = 0xFFFFFB /* 5400 K, 255, 255, 251 */,
  /// 6000 Kelvin
  DirectSunlight = 0xFFFFFF /* 6000 K, 255, 255, 255 */,
  /// 7000 Kelvin
  OvercastSky = 0xC9E2FF /* 7000 K, 201, 226, 255 */,
  /// 20000 Kelvin
  ClearBlueSky = 0x409CFF /* 20000 K, 64, 156, 255 */,
  // @}

  // Gaseous Light Sources
  // @{
  /// Warm (yellower) flourescent light bulbs
  WarmFluorescent = 0xFFF4E5 /* 0 K, 255, 244, 229 */,
  /// Standard flourescent light bulbs
  StandardFluorescent = 0xF4FFFA /* 0 K, 244, 255, 250 */,
  /// Cool white (bluer) flourescent light bulbs
  CoolWhiteFluorescent = 0xD4EBFF /* 0 K, 212, 235, 255 */,
  /// Full spectrum flourescent light bulbs
  FullSpectrumFluorescent = 0xFFF4F2 /* 0 K, 255, 244, 242 */,
  /// Grow light flourescent light bulbs
  GrowLightFluorescent = 0xFFEFF7 /* 0 K, 255, 239, 247 */,
  /// Black light flourescent light bulbs
  BlackLightFluorescent = 0xA700FF /* 0 K, 167, 0, 255 */,
  /// Mercury vapor light bulbs
  MercuryVapor = 0xD8F7FF /* 0 K, 216, 247, 255 */,
  /// Sodium vapor light bulbs
  SodiumVapor = 0xFFD1B2 /* 0 K, 255, 209, 178 */,
  /// Metal-halide light bulbs
  MetalHalide = 0xF2FCFF /* 0 K, 242, 252, 255 */,
  /// High-pressure sodium light bulbs
  HighPressureSodium = 0xFFB74C /* 0 K, 255, 183, 76 */,
  // @}

  /// Uncorrected temperature (0xFFFFFF)
  UncorrectedTemperature = 0xFFFFFF /* 255, 255, 255 */
} ColorTemperature;

typedef enum
{
  AliceBlue = 0xF0F8FF,            ///< @htmlcolorblock{F0F8FF}
  Amethyst = 0x9966CC,             ///< @htmlcolorblock{9966CC}
  AntiqueWhite = 0xFAEBD7,         ///< @htmlcolorblock{FAEBD7}
  Aqua = 0x00FFFF,                 ///< @htmlcolorblock{00FFFF}
  Aquamarine = 0x7FFFD4,           ///< @htmlcolorblock{7FFFD4}
  Azure = 0xF0FFFF,                ///< @htmlcolorblock{F0FFFF}
  Beige = 0xF5F5DC,                ///< @htmlcolorblock{F5F5DC}
  Bisque = 0xFFE4C4,               ///< @htmlcolorblock{FFE4C4}
  Black = 0x000000,                ///< @htmlcolorblock{000000}
  BlanchedAlmond = 0xFFEBCD,       ///< @htmlcolorblock{FFEBCD}
  Blue = 0x0000FF,                 ///< @htmlcolorblock{0000FF}
  BlueViolet = 0x8A2BE2,           ///< @htmlcolorblock{8A2BE2}
  Brown = 0xA52A2A,                ///< @htmlcolorblock{A52A2A}
  BurlyWood = 0xDEB887,            ///< @htmlcolorblock{DEB887}
  CadetBlue = 0x5F9EA0,            ///< @htmlcolorblock{5F9EA0}
  Chartreuse = 0x7FFF00,           ///< @htmlcolorblock{7FFF00}
  Chocolate = 0xD2691E,            ///< @htmlcolorblock{D2691E}
  Coral = 0xFF7F50,                ///< @htmlcolorblock{FF7F50}
  CornflowerBlue = 0x6495ED,       ///< @htmlcolorblock{6495ED}
  Cornsilk = 0xFFF8DC,             ///< @htmlcolorblock{FFF8DC}
  Crimson = 0xDC143C,              ///< @htmlcolorblock{DC143C}
  Cyan = 0x00FFFF,                 ///< @htmlcolorblock{00FFFF}
  DarkBlue = 0x00008B,             ///< @htmlcolorblock{00008B}
  DarkCyan = 0x008B8B,             ///< @htmlcolorblock{008B8B}
  DarkGoldenrod = 0xB8860B,        ///< @htmlcolorblock{B8860B}
  DarkGray = 0xA9A9A9,             ///< @htmlcolorblock{A9A9A9}
  DarkGrey = 0xA9A9A9,             ///< @htmlcolorblock{A9A9A9}
  DarkGreen = 0x006400,            ///< @htmlcolorblock{006400}
  DarkKhaki = 0xBDB76B,            ///< @htmlcolorblock{BDB76B}
  DarkMagenta = 0x8B008B,          ///< @htmlcolorblock{8B008B}
  DarkOliveGreen = 0x556B2F,       ///< @htmlcolorblock{556B2F}
  DarkOrange = 0xFF8C00,           ///< @htmlcolorblock{FF8C00}
  DarkOrchid = 0x9932CC,           ///< @htmlcolorblock{9932CC}
  DarkRed = 0x8B0000,              ///< @htmlcolorblock{8B0000}
  DarkSalmon = 0xE9967A,           ///< @htmlcolorblock{E9967A}
  DarkSeaGreen = 0x8FBC8F,         ///< @htmlcolorblock{8FBC8F}
  DarkSlateBlue = 0x483D8B,        ///< @htmlcolorblock{483D8B}
  DarkSlateGray = 0x2F4F4F,        ///< @htmlcolorblock{2F4F4F}
  DarkSlateGrey = 0x2F4F4F,        ///< @htmlcolorblock{2F4F4F}
  DarkTurquoise = 0x00CED1,        ///< @htmlcolorblock{00CED1}
  DarkViolet = 0x9400D3,           ///< @htmlcolorblock{9400D3}
  DeepPink = 0xFF1493,             ///< @htmlcolorblock{FF1493}
  DeepSkyBlue = 0x00BFFF,          ///< @htmlcolorblock{00BFFF}
  DimGray = 0x696969,              ///< @htmlcolorblock{696969}
  DimGrey = 0x696969,              ///< @htmlcolorblock{696969}
  DodgerBlue = 0x1E90FF,           ///< @htmlcolorblock{1E90FF}
  FireBrick = 0xB22222,            ///< @htmlcolorblock{B22222}
  FloralWhite = 0xFFFAF0,          ///< @htmlcolorblock{FFFAF0}
  ForestGreen = 0x228B22,          ///< @htmlcolorblock{228B22}
  Fuchsia = 0xFF00FF,              ///< @htmlcolorblock{FF00FF}
  Gainsboro = 0xDCDCDC,            ///< @htmlcolorblock{DCDCDC}
  GhostWhite = 0xF8F8FF,           ///< @htmlcolorblock{F8F8FF}
  Gold = 0xFFD700,                 ///< @htmlcolorblock{FFD700}
  Goldenrod = 0xDAA520,            ///< @htmlcolorblock{DAA520}
  Gray = 0x808080,                 ///< @htmlcolorblock{808080}
  Grey = 0x808080,                 ///< @htmlcolorblock{808080}
  Green = 0x008000,                ///< @htmlcolorblock{008000}
  GreenYellow = 0xADFF2F,          ///< @htmlcolorblock{ADFF2F}
  Honeydew = 0xF0FFF0,             ///< @htmlcolorblock{F0FFF0}
  HotPink = 0xFF69B4,              ///< @htmlcolorblock{FF69B4}
  IndianRed = 0xCD5C5C,            ///< @htmlcolorblock{CD5C5C}
  Indigo = 0x4B0082,               ///< @htmlcolorblock{4B0082}
  Ivory = 0xFFFFF0,                ///< @htmlcolorblock{FFFFF0}
  Khaki = 0xF0E68C,                ///< @htmlcolorblock{F0E68C}
  Lavender = 0xE6E6FA,             ///< @htmlcolorblock{E6E6FA}
  LavenderBlush = 0xFFF0F5,        ///< @htmlcolorblock{FFF0F5}
  LawnGreen = 0x7CFC00,            ///< @htmlcolorblock{7CFC00}
  LemonChiffon = 0xFFFACD,         ///< @htmlcolorblock{FFFACD}
  LightBlue = 0xADD8E6,            ///< @htmlcolorblock{ADD8E6}
  LightCoral = 0xF08080,           ///< @htmlcolorblock{F08080}
  LightCyan = 0xE0FFFF,            ///< @htmlcolorblock{E0FFFF}
  LightGoldenrodYellow = 0xFAFAD2, ///< @htmlcolorblock{FAFAD2}
  LightGreen = 0x90EE90,           ///< @htmlcolorblock{90EE90}
  LightGrey = 0xD3D3D3,            ///< @htmlcolorblock{D3D3D3}
  LightPink = 0xFFB6C1,            ///< @htmlcolorblock{FFB6C1}
  LightSalmon = 0xFFA07A,          ///< @htmlcolorblock{FFA07A}
  LightSeaGreen = 0x20B2AA,        ///< @htmlcolorblock{20B2AA}
  LightSkyBlue = 0x87CEFA,         ///< @htmlcolorblock{87CEFA}
  LightSlateGray = 0x778899,       ///< @htmlcolorblock{778899}
  LightSlateGrey = 0x778899,       ///< @htmlcolorblock{778899}
  LightSteelBlue = 0xB0C4DE,       ///< @htmlcolorblock{B0C4DE}
  LightYellow = 0xFFFFE0,          ///< @htmlcolorblock{FFFFE0}
  Lime = 0x00FF00,                 ///< @htmlcolorblock{00FF00}
  LimeGreen = 0x32CD32,            ///< @htmlcolorblock{32CD32}
  Linen = 0xFAF0E6,                ///< @htmlcolorblock{FAF0E6}
  Magenta = 0xFF00FF,              ///< @htmlcolorblock{FF00FF}
  Maroon = 0x800000,               ///< @htmlcolorblock{800000}
  MediumAquamarine = 0x66CDAA,     ///< @htmlcolorblock{66CDAA}
  MediumBlue = 0x0000CD,           ///< @htmlcolorblock{0000CD}
  MediumOrchid = 0xBA55D3,         ///< @htmlcolorblock{BA55D3}
  MediumPurple = 0x9370DB,         ///< @htmlcolorblock{9370DB}
  MediumSeaGreen = 0x3CB371,       ///< @htmlcolorblock{3CB371}
  MediumSlateBlue = 0x7B68EE,      ///< @htmlcolorblock{7B68EE}
  MediumSpringGreen = 0x00FA9A,    ///< @htmlcolorblock{00FA9A}
  MediumTurquoise = 0x48D1CC,      ///< @htmlcolorblock{48D1CC}
  MediumVioletRed = 0xC71585,      ///< @htmlcolorblock{C71585}
  MidnightBlue = 0x191970,         ///< @htmlcolorblock{191970}
  MintCream = 0xF5FFFA,            ///< @htmlcolorblock{F5FFFA}
  MistyRose = 0xFFE4E1,            ///< @htmlcolorblock{FFE4E1}
  Moccasin = 0xFFE4B5,             ///< @htmlcolorblock{FFE4B5}
  NavajoWhite = 0xFFDEAD,          ///< @htmlcolorblock{FFDEAD}
  Navy = 0x000080,                 ///< @htmlcolorblock{000080}
  OldLace = 0xFDF5E6,              ///< @htmlcolorblock{FDF5E6}
  Olive = 0x808000,                ///< @htmlcolorblock{808000}
  OliveDrab = 0x6B8E23,            ///< @htmlcolorblock{6B8E23}
  Orange = 0xFFA500,               ///< @htmlcolorblock{FFA500}
  OrangeRed = 0xFF4500,            ///< @htmlcolorblock{FF4500}
  Orchid = 0xDA70D6,               ///< @htmlcolorblock{DA70D6}
  PaleGoldenrod = 0xEEE8AA,        ///< @htmlcolorblock{EEE8AA}
  PaleGreen = 0x98FB98,            ///< @htmlcolorblock{98FB98}
  PaleTurquoise = 0xAFEEEE,        ///< @htmlcolorblock{AFEEEE}
  PaleVioletRed = 0xDB7093,        ///< @htmlcolorblock{DB7093}
  PapayaWhip = 0xFFEFD5,           ///< @htmlcolorblock{FFEFD5}
  PeachPuff = 0xFFDAB9,            ///< @htmlcolorblock{FFDAB9}
  Peru = 0xCD853F,                 ///< @htmlcolorblock{CD853F}
  Pink = 0xFFC0CB,                 ///< @htmlcolorblock{FFC0CB}
  Plaid = 0xCC5533,                ///< @htmlcolorblock{CC5533}
  Plum = 0xDDA0DD,                 ///< @htmlcolorblock{DDA0DD}
  PowderBlue = 0xB0E0E6,           ///< @htmlcolorblock{B0E0E6}
  Purple = 0x800080,               ///< @htmlcolorblock{800080}
  Red = 0xFF0000,                  ///< @htmlcolorblock{FF0000}
  RosyBrown = 0xBC8F8F,            ///< @htmlcolorblock{BC8F8F}
  RoyalBlue = 0x4169E1,            ///< @htmlcolorblock{4169E1}
  SaddleBrown = 0x8B4513,          ///< @htmlcolorblock{8B4513}
  Salmon = 0xFA8072,               ///< @htmlcolorblock{FA8072}
  SandyBrown = 0xF4A460,           ///< @htmlcolorblock{F4A460}
  SeaGreen = 0x2E8B57,             ///< @htmlcolorblock{2E8B57}
  Seashell = 0xFFF5EE,             ///< @htmlcolorblock{FFF5EE}
  Sienna = 0xA0522D,               ///< @htmlcolorblock{A0522D}
  Silver = 0xC0C0C0,               ///< @htmlcolorblock{C0C0C0}
  SkyBlue = 0x87CEEB,              ///< @htmlcolorblock{87CEEB}
  SlateBlue = 0x6A5ACD,            ///< @htmlcolorblock{6A5ACD}
  SlateGray = 0x708090,            ///< @htmlcolorblock{708090}
  SlateGrey = 0x708090,            ///< @htmlcolorblock{708090}
  Snow = 0xFFFAFA,                 ///< @htmlcolorblock{FFFAFA}
  SpringGreen = 0x00FF7F,          ///< @htmlcolorblock{00FF7F}
  SteelBlue = 0x4682B4,            ///< @htmlcolorblock{4682B4}
  Tan = 0xD2B48C,                  ///< @htmlcolorblock{D2B48C}
  Teal = 0x008080,                 ///< @htmlcolorblock{008080}
  Thistle = 0xD8BFD8,              ///< @htmlcolorblock{D8BFD8}
  Tomato = 0xFF6347,               ///< @htmlcolorblock{FF6347}
  Turquoise = 0x40E0D0,            ///< @htmlcolorblock{40E0D0}
  Violet = 0xEE82EE,               ///< @htmlcolorblock{EE82EE}
  Wheat = 0xF5DEB3,                ///< @htmlcolorblock{F5DEB3}
  White = 0xFFFFFF,                ///< @htmlcolorblock{FFFFFF}
  WhiteSmoke = 0xF5F5F5,           ///< @htmlcolorblock{F5F5F5}
  Yellow = 0xFFFF00,               ///< @htmlcolorblock{FFFF00}
  YellowGreen = 0x9ACD32,          ///< @htmlcolorblock{9ACD32}

  // LED RGB color that roughly approximates
  // the color of incandescent fairy lights,
  // assuming that you're using FastLED
  // color correction on your LEDs (recommended).
  FairyLight = 0xFFE42D, ///< @htmlcolorblock{FFE42D}

  // If you are using no color correction, use this
  FairyLightNCC = 0xFF9D2A ///< @htmlcolorblock{FFE42D}

} HTMLColorCode;

extern const palette_t PaletteCloudColors;     // blue/white palette
extern const palette_t PaletteLavaColors;      // red orange black
extern const palette_t PaletteFlameColors;     // lots of orange, touches of gold/yellow
extern const palette_t PaletteOceanColors;     // Ocean colors, blues and whites
extern const palette_t PaletteWaterColors;     // Water colors, blues
extern const palette_t PaletteForestColors;    // Forest colors, greens
extern const palette_t PaletteRainbowColors;   // HSV Rainbow
extern const palette_t PalettePartyColors;     // basically, HSV with no green. looks better when lighing people
extern const palette_t PaletteBlackBodyColors; // Black body radiation
extern const palette_t PaletteHeatColors;
extern const palette_t PaletteAuroraColors;

/**
 * \brief Return a color from a palette
 * \param[in] index from 0 to 255, the index of color we want from the palette
 * \param[in] palette The palette to sample from. Values are interpolated
 * \param[in] brightness The brighness of the color, default is max at 255
 * \return The desired color
 */
uint32_t get_color_from_palette(const uint8_t index, const palette_t& palette, const uint8_t brightness = 255);
uint32_t get_color_from_palette(const uint16_t index, const palette_t& palette, const uint8_t brightness = 255);

#endif

#endif

#ifndef COLOR_SPACE_H
#define COLOR_SPACE_H

#include "utils.h"

namespace utils::ColorSpace {

class Base
{
public:
  /// virtual destructor
  virtual ~Base() = default;
  /// return the rgb of this color
  virtual COLOR get_rgb() const = 0;
};

class XYZ : public Base
{
public:
  /// Construct from RGB
  XYZ(const COLOR& c) { from_rgb(c); };
  /// Construct from values
  XYZ(double x, double y, double z) : x(x), y(y), z(z) {};
  /// Return the white value
  static XYZ get_white()
  {
    static const XYZ white(95.047, 100.000, 108.883);
    return white;
  }

  /// get the rgb form (for display)
  COLOR get_rgb() const override;
  /// construct from an RGB representation
  void from_rgb(const COLOR& rgb);

  double x; ///< Color space X
  double y; ///< Color space Y
  double z; ///< Color space Z
};

class RGB : public Base
{
public:
  /// construct from values
  RGB(uint8_t red, uint8_t green, uint8_t blue)
  {
    _color.red = red;
    _color.green = green;
    _color.blue = blue;
  }
  /// construct from color
  RGB(const uint32_t color) { _color.color = color; }
  /// Get the RGB representation
  COLOR get_rgb() const override { return _color; }

private:
  COLOR _color;
};

// define some colors
static const RGB RED(255, 0, 0);
static const RGB GREEN(0, 255, 0);
static const RGB BLUE(0, 0, 255);
static const RGB WHITE(255, 255, 255);

static const RGB BLACK(0, 0, 0);
static const RGB FUSHIA(255, 0, 255);
static const RGB TEAL(0, 255, 255);
static const RGB YELLOW(200, 255, 0);
static const RGB PINK(255, 20, 147);
static const RGB TOMATO(255, 99, 71);
static const RGB ORANGE(255, 140, 0);
static const RGB DARK_ORANGE(255, 90, 0);
static const RGB PURPLE(128, 0, 128);

class HSV : public Base
{
public:
  /// Construct from RGB
  HSV(const COLOR& c) { from_rgb(c); };
  /// Construct from values
  HSV(double h, double s, double v) : h(h), s(s), v(v) {};

  /// get the rgb form (for display)
  COLOR get_rgb() const override;
  /// construct from an RGB representation
  void from_rgb(const COLOR& rgb);

  /// get the hue scaled between 0 and UINT16_MAX
  uint16_t get_scaled_hue() const { return static_cast<uint16_t>(h / 360.0 * UINT16_MAX); }

  double h; ///< Hue
  double s; ///< Saturation
  double v; ///< value
};

class LAB : public Base
{
public:
  /// Construct from RGB
  LAB(const COLOR& c) { from_rgb(c); };
  /// Construct from values
  LAB(const double l, const double a, const double b) : l(l), a(a), b(b) {};

  /// get the rgb form (for display)
  COLOR get_rgb() const override;
  /// construct from an RGB representation
  void from_rgb(const COLOR& rgb);

  double l; ///< Luminance
  double a; ///< Alpha
  double b; ///< Brightness
};

class LCH : public Base
{
public:
  /// Construct from RGB
  LCH(const COLOR& c) { from_rgb(c); };
  /// Construct from values
  LCH(const double l, const double c, const double h) : l(l), c(c), h(h) {};

  /// get the rgb form (for display)
  COLOR get_rgb() const override;
  /// construct from an RGB representation
  void from_rgb(const COLOR& rgb);

  /// get the hue scaled between 0 and UINT16_MAX
  uint16_t get_scaled_hue() const { return static_cast<uint16_t>(h / 360.0 * UINT16_MAX); }

  double l; ///< Luminance
  double c; ///< Chroma
  double h; ///< Hue 0 - 360
};

class OKLAB : public Base
{
public:
  /// Construct from RGB
  OKLAB(const COLOR& c) { from_rgb(c); };
  /// Construct from values
  OKLAB(const double l, const double a, const double b) : l(l), a(a), b(b) {};

  /// get the rgb form (for display)
  COLOR get_rgb() const override;

  /// construct from an RGB representation
  void from_rgb(const COLOR& rgb);

  double l; ///< Luminance
  double a; ///< Alpha
  double b; ///< Brightness
};

class OKLCH : public Base
{
public:
  /// Construct from RGB
  OKLCH(const COLOR& c) { from_rgb(c); };
  /// Construct from values
  OKLCH(const double l, const double c, const double h) : l(l), c(c), h(h) {};

  /// get the rgb form (for display)
  COLOR get_rgb() const override;

  /// construct from an RGB representation
  void from_rgb(const COLOR& rgb);

  /// get the hue scaled between 0 and UINT16_MAX
  uint16_t get_scaled_hue() const { return static_cast<uint16_t>(h / 360.0 * UINT16_MAX); }

  double l; ///< Luminance
  double c; ///< Chroma
  double h; ///< Hue 0 - 360
};

} // namespace utils::ColorSpace

#endif

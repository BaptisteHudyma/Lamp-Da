#ifndef COLOR_SPACE_H
#define COLOR_SPACE_H

#include "utils.h"

namespace utils::ColorSpace {

class Base
{
public:
  virtual ~Base() = default;
  virtual COLOR get_rgb() const = 0;
};

class XYZ : public Base
{
public:
  XYZ(const COLOR& c) { from_rgb(c); };
  XYZ(double x, double y, double z) : x(x), y(y), z(z) {};

  static XYZ get_white()
  {
    static const XYZ white(95.047, 100.000, 108.883);
    return white;
  }

  COLOR get_rgb() const override;

  void from_rgb(const COLOR& rgb);

  double x;
  double y;
  double z;
};

class RGB : public Base
{
public:
  RGB(uint8_t red, uint8_t green, uint8_t blue)
  {
    _color.red = red;
    _color.green = green;
    _color.blue = blue;
  }

  RGB(const uint32_t color) { _color.color = color; }

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
  HSV(const COLOR& c) { from_rgb(c); };
  HSV(double h, double s, double v) : h(h), s(s), v(v) {};

  COLOR get_rgb() const override;

  void from_rgb(const COLOR& rgb);

  uint16_t get_scaled_hue() const { return static_cast<uint16_t>(h / 360.0 * UINT16_MAX); }

  double h;
  double s;
  double v;
};

class LAB : public Base
{
public:
  LAB(const COLOR& c) { from_rgb(c); };
  LAB(const double l, const double a, const double b) : l(l), a(a), b(b) {};

  // get the rgb form (for display)
  COLOR get_rgb() const override;

  void from_rgb(const COLOR& rgb);

  double l;
  double a;
  double b;
};

class LCH : public Base
{
public:
  LCH(const COLOR& c) { from_rgb(c); };
  LCH(const double l, const double c, const double h) : l(l), c(c), h(h) {};

  // get the rgb form (for display)
  COLOR get_rgb() const override;

  void from_rgb(const COLOR& rgb);

  uint16_t get_scaled_hue() const { return static_cast<uint16_t>(h / 360.0 * UINT16_MAX); }

  double l;
  double c;
  double h; // 0 - 360
};

class OKLAB : public Base
{
public:
  OKLAB(const COLOR& c) { from_rgb(c); };
  OKLAB(const double l, const double a, const double b) : l(l), a(a), b(b) {};

  // get the rgb form (for display)
  COLOR get_rgb() const override;

  void from_rgb(const COLOR& rgb);

  double l;
  double a;
  double b;
};

class OKLCH : public Base
{
public:
  OKLCH(const COLOR& c) { from_rgb(c); };
  OKLCH(const double l, const double c, const double h) : l(l), c(c), h(h) {};

  // get the rgb form (for display)
  COLOR get_rgb() const override;

  void from_rgb(const COLOR& rgb);

  uint16_t get_scaled_hue() const { return static_cast<uint16_t>(h / 360.0 * UINT16_MAX); }

  double l;
  double c;
  double h; // 0 - 360
};

} // namespace utils::ColorSpace

#endif

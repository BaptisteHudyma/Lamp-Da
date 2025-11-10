#include "colorspace.h"

#include "utils.h"

namespace utils::ColorSpace {

COLOR XYZ::get_rgb() const
{
  double x = this->x / 100.0;
  double y = this->y / 100.0;
  double z = this->z / 100.0;

  double r = x * 3.2404542 + y * -1.5371385 + z * -0.4985314;
  double g = x * -0.9692660 + y * 1.8760108 + z * 0.0415560;
  double b = x * 0.0556434 + y * -0.2040259 + z * 1.0572252;

  r = ((r > 0.0031308) ? (1.055 * std::pow(r, 1 / 2.4) - 0.055) : (12.92 * r)) * 255.0;
  g = ((g > 0.0031308) ? (1.055 * std::pow(g, 1 / 2.4) - 0.055) : (12.92 * g)) * 255.0;
  b = ((b > 0.0031308) ? (1.055 * std::pow(b, 1 / 2.4) - 0.055) : (12.92 * b)) * 255.0;

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

  this->x = r * 0.4124564 + g * 0.3575761 + b * 0.1804375;
  this->y = r * 0.2126729 + g * 0.7151522 + b * 0.0721750;
  this->z = r * 0.0193339 + g * 0.1191920 + b * 0.9503041;
}

COLOR HSV::get_rgb() const
{
  int range = (int)std::floor(this->h / 60.0);
  double c = this->v * this->s;
  double x = c * (1 - abs(std::fmod(this->h / 60.0, 2) - 1.0));
  double m = this->v - c;

  COLOR out;
  switch (range)
  {
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
    default: // case 5:
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

  double _min = min<double>(r, min<double>(g, b));
  double _max = max<double>(r, max<double>(g, b));
  double delta = _max - _min;

  this->v = _max;
  this->s = (_max > 1e-3) ? (delta / _max) : 0;

  if (delta == 0)
  {
    this->h = 0;
  }
  else
  {
    if (r == _max)
    {
      this->h = (g - b) / delta;
    }
    else if (g == _max)
    {
      this->h = 2 + (b - r) / delta;
    }
    else if (b == _max)
    {
      this->h = 4 + (r - g) / delta;
    }

    this->h *= 60;
    this->h = std::fmod(this->h + 360, 360);
  }
}

// get the rgb form (for display)
COLOR LAB::get_rgb() const
{
  const XYZ& white = XYZ::get_white();

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
  const XYZ& white = XYZ::get_white();

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
  const double newH = this->h * c_PI / 180;

  LAB lab(this->l, std::cos(newH) * this->c, std::sin(newH) * this->c);
  return lab.get_rgb();
}

void LCH::from_rgb(const COLOR& rgb)
{
  LAB lab(rgb);

  double l = lab.l;
  double c = std::sqrt(lab.a * lab.a + lab.b * lab.b);
  double h = std::atan2(lab.b, lab.a);

  h = h / c_PI * 180;
  if (h < 0)
  {
    h += 360;
  }
  else if (h >= 360)
  {
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

  double r = 4.0767245293 * l - 3.3072168827 * m + 0.2307590544 * s;
  double g = -1.2681437731 * l + 2.6093323231 * m - 0.3411344290 * s;
  double b = -0.0041119885 * l - 0.7034763098 * m + 1.7068625689 * s;

  COLOR out;
  out.red = ((r > 0.0031308) ? (1.055 * std::pow(r, 1 / 2.4) - 0.055) : (12.92 * r)) * 255.0;
  out.green = ((g > 0.0031308) ? (1.055 * std::pow(g, 1 / 2.4) - 0.055) : (12.92 * g)) * 255.0;
  out.blue = ((b > 0.0031308) ? (1.055 * std::pow(b, 1 / 2.4) - 0.055) : (12.92 * b)) * 255.0;
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
  const double newH = this->h * c_PI / 180.0;

  OKLAB lab(this->l, std::cos(newH) * this->c, std::sin(newH) * this->c);
  return lab.get_rgb();
}

void OKLCH::from_rgb(const COLOR& rgb)
{
  OKLAB lab(rgb);

  double l = lab.l;
  double c = std::sqrt(lab.a * lab.a + lab.b * lab.b);
  double h = std::atan2(lab.b, lab.a);

  h = h / c_PI * 180;
  if (h < 0)
  {
    h += 360;
  }
  else if (h >= 360)
  {
    h -= 360;
  }

  this->l = l;
  this->c = c;
  this->h = h;
}

} // namespace utils::ColorSpace

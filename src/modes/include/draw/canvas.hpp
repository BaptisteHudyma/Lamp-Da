// canvas.hpp
// ---------------------------------------------------------------------------
// Flat "unrolled cylinder" canvas + compile-time sampler for a helix LED strip.
//
//   App draws on Canvas (W x H, one packed uint16_t per pixel, x wraps, y clipped)
//     -> render()  (constexpr sparse Gaussian footprint per LED)
//     -> Rgb8 LED buffer
//
// No gamma, no dithering: colours are blended as they are stored.
//
// Requirements : C++17, no heap, no <cmath>, no exceptions, no RTTI.
// Compiler note: heavy constexpr evaluation. If needed, raise the limits:
//     GCC   : -fconstexpr-ops-limit=500000000 -fconstexpr-loop-limit=1000000
//     Clang : -fconstexpr-steps=500000000
//
// Geometry assumptions (edit config / geom::u() / geom::v() to change them):
//   * LEDs sit on the outer face of the strip, at radius = body + strip height.
//   * Adjacent turns touch: vertical pitch of the helix = strip width.
//   * LED spacing along the strip = 1000 / ledByMeter.
//   * Unrolled sheet: u = arc length around the cylinder (wraps), v = height
//     (downward, like image rows). Canvas pixel (0,0) is top-left.
//   * Calibrated LED positions: replace geom::u(i) / geom::v(i) by a constexpr
//     table lookup (units: mm). Nothing else needs to change.
// ---------------------------------------------------------------------------
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "src/user/constants.h"

namespace lampda::modes::draw {
namespace canvas {

using LampTy = hardware::LampTy;

// ============================ Configuration ================================
namespace config {
// --- Tunables ---
inline constexpr float canvasTargetPitch_mm = 3.0f;  // wanted canvas pixel size (<= LED pitch / 2)
inline constexpr float footprintSigmaFactor = 0.6f;  // sigma = factor * min(LED spacing, turn pitch)
inline constexpr float footprintCutoffSigmas = 2.0f; // footprint radius = cutoff * sigma
inline constexpr float startOffset_mm = 0.0f;        // u position of LED 0 (rotates the helix)
inline constexpr bool stripStartsAtTop = true;       // LED 0 at the top (true) or bottom (false)

// Seen from above, the strip turns clockwise (true) or anticlockwise (false)
// as the LED index increases. Canvas x always increases clockwise seen from above.
inline constexpr bool helixClockwise = true;

// Isolated pixel protection. Each LED also computes a "peak" term where its strongest tap counts for
// 100% (other taps are scaled relative to it). The LED shows max(weighted average, peak term) per channel,
// so a lone pixel is no longer diluted by black neighbours.
//   1.0 = a pixel at an LED's strongest tap shows at full brightness (default)
//   <1  = weaker effect, more blur      0.0 = disabled (pure weighted average)
inline constexpr float peakStrength = 1.0f;
} // namespace config

// ============================ Colour types =================================
struct Rgb8
{
  std::uint8_t r = 0, g = 0, b = 0;

  static Rgb8 from_uint32(const uint32_t color)
  {
    Rgb8 rgb;
    rgb.r = (uint8_t)((color >> 16) & 0xff);
    rgb.g = (uint8_t)((color >> 8) & 0xff);
    rgb.b = (uint8_t)(color & 0xff);
    return rgb;
  }

  uint32_t to_uint32() const
  {
    uint32_t colorU = r << 16 | g << 8 | b;
    return colorU;
  }
};

// Packed 16 bit pixel <-> 3 x uint8_t. ADAPT THIS to your own conversion functions.
// Contract: unpack(0xFFFF) must give {255,255,255} and unpack(0) must give {0,0,0},
// so that a uniform canvas is reproduced exactly.
// Default implementation: RGB565 with bit replication.
struct Rgb565Codec
{
  static constexpr std::uint16_t pack(Rgb8 c)
  {
    return static_cast<std::uint16_t>(((c.r & 0xF8u) << 8) | ((c.g & 0xFCu) << 3) | (c.b >> 3));
  }
  static constexpr Rgb8 unpack(std::uint16_t v)
  {
    const unsigned r5 = (v >> 11) & 0x1Fu, g6 = (v >> 5) & 0x3Fu, b5 = v & 0x1Fu;
    return Rgb8 {static_cast<std::uint8_t>((r5 << 3) | (r5 >> 2)),
                 static_cast<std::uint8_t>((g6 << 2) | (g6 >> 4)),
                 static_cast<std::uint8_t>((b5 << 3) | (b5 >> 2))};
  }
};
using Codec = Rgb565Codec; // <- replace with your own codec type

// ======================= Constexpr math (no <cmath>) =======================
namespace cx {
inline constexpr double kPi = c_PI;
inline constexpr double kLn2 = 0.69314718055994530942;

constexpr double floor(double x)
{
  const long long i = static_cast<long long>(x);
  return (static_cast<double>(i) > x) ? static_cast<double>(i - 1) : static_cast<double>(i);
}
constexpr double ceil(double x)
{
  const long long i = static_cast<long long>(x);
  return (static_cast<double>(i) < x) ? static_cast<double>(i + 1) : static_cast<double>(i);
}
constexpr double sqrt(double x)
{
  if (x <= 0.0)
    return 0.0;
  double g = (x + 1.0) * 0.5;
  for (int i = 0; i < 60; ++i)
    g = 0.5 * (g + x / g);
  return g;
}
constexpr double exp(double x)
{
  if (x < -700.0)
    return 0.0;
  const double k = floor(x / kLn2 + 0.5);
  const double r = x - k * kLn2; // |r| <= ln2 / 2
  double term = 1.0, sum = 1.0;
  for (int n = 1; n < 20; ++n)
  {
    term *= r / n;
    sum += term;
  }
  int ki = static_cast<int>(k);
  while (ki > 0)
  {
    sum *= 2.0;
    --ki;
  }
  while (ki < 0)
  {
    sum *= 0.5;
    ++ki;
  }
  return sum;
}
constexpr double wrap(double x, double period) { return x - floor(x / period) * period; }
} // namespace cx

// ============================== Geometry ===================================
namespace geom {
inline constexpr double turnSign = config::helixClockwise ? 1.0 : -1.0;

inline constexpr std::size_t N = LampTy::ledCount;

inline constexpr double ledRadius_mm = double(LampTy::lampBodyRadius_mm);
inline constexpr double circumference_mm = 2.0 * cx::kPi * ledRadius_mm;
inline constexpr double turnPitch_mm = LampTy::ledStripWidth_mm; // height gained per turn
inline constexpr double turnLength_mm = cx::sqrt(circumference_mm * circumference_mm + turnPitch_mm * turnPitch_mm);

// Step between two consecutive LEDs on the unrolled sheet.
inline constexpr double du_mm = LampTy::ledSize_mm * circumference_mm / turnLength_mm;
inline constexpr double dv_mm = LampTy::ledSize_mm * turnPitch_mm / turnLength_mm;
inline constexpr double helixHeight_mm = (N - 1) * dv_mm + LampTy::ledStripWidth_mm;

// Canvas: integer width so that x wraps exactly; square pixels.

// Here we define a fixed canvas size so that all lamp type share the same displays, but it could be generated from the
// physical characteristics
inline constexpr int W = 54;
inline constexpr int H = 41;

inline constexpr double pitch_mm = circumference_mm / W;
inline constexpr double canvasHeight_mm = H * pitch_mm;
inline constexpr double margin_mm = (canvasHeight_mm - helixHeight_mm) * 0.5;

// LED centre on the unrolled sheet, in mm.
constexpr double u(std::size_t i) { return cx::wrap(turnSign * i * du_mm + config::startOffset_mm, circumference_mm); }
constexpr double v(std::size_t i)
{
  const double off = LampTy::ledStripWidth_mm * 0.5 + i * dv_mm;
  return config::stripStartsAtTop ? margin_mm + off : canvasHeight_mm - margin_mm - off;
}
} // namespace geom

inline constexpr int kCanvasWidth = geom::W;
inline constexpr int kCanvasHeight = geom::H;
inline constexpr std::size_t kLedCount = geom::N;

static_assert(kCanvasWidth <= 255 && kCanvasHeight <= 255, "Tap coordinates are stored on 8 bits");

// ========================= Compile-time tap tables =========================
namespace detail {
inline constexpr double kSigma_mm = config::footprintSigmaFactor *
                                    (LampTy::ledSize_mm < geom::turnPitch_mm ? LampTy::ledSize_mm : geom::turnPitch_mm);
inline constexpr double kRadius_mm = config::footprintCutoffSigmas * kSigma_mm;
inline constexpr double kRadiusPx = kRadius_mm / geom::pitch_mm;
static_assert(2.0 * kRadiusPx + 2.0 < geom::W, "Footprint must be narrower than the canvas circumference");

struct Tap
{
  std::uint8_t x;  // wrapped column
  std::uint8_t y;  // row
  std::uint16_t w; // weight, Q0.16 (sum per LED == 65536)
};

constexpr double gauss(double d2_mm2) { return cx::exp(-d2_mm2 / (2.0 * kSigma_mm * kSigma_mm)); }

// Calls f(x, y, d2_mm2) for each canvas pixel inside the footprint of LED `led`.
template<class F> constexpr void forEachTap(std::size_t led, F&& f)
{
  const double cu = geom::u(led) / geom::pitch_mm; // LED centre in pixel units
  const double cv = geom::v(led) / geom::pitch_mm;
  int x0 = static_cast<int>(cx::ceil(cu - kRadiusPx - 0.5));
  int x1 = static_cast<int>(cx::floor(cu + kRadiusPx - 0.5));
  int y0 = static_cast<int>(cx::ceil(cv - kRadiusPx - 0.5));
  int y1 = static_cast<int>(cx::floor(cv + kRadiusPx - 0.5));
  if (y0 < 0)
    y0 = 0;
  if (y1 > geom::H - 1)
    y1 = geom::H - 1;
  for (int y = y0; y <= y1; ++y)
  {
    for (int x = x0; x <= x1; ++x)
    {
      const double dx = (x + 0.5 - cu) * geom::pitch_mm;
      const double dy = (y + 0.5 - cv) * geom::pitch_mm;
      const double d2 = dx * dx + dy * dy;
      if (d2 > kRadius_mm * kRadius_mm)
        continue;
      int xw = x;
      if (xw < 0)
        xw += geom::W;
      if (xw >= geom::W)
        xw -= geom::W;
      f(xw, y, d2);
    }
  }
}

// Pass 1: how many taps per LED (gives the exact storage size).
struct Counts
{
  std::array<std::uint32_t, kLedCount + 1> offset {};
  std::uint32_t total = 0;
  std::uint32_t minPerLed = 0xFFFFFFFFu;
};

constexpr Counts countTaps()
{
  Counts c {};
  std::uint32_t pos = 0;
  for (std::size_t i = 0; i < kLedCount; ++i)
  {
    c.offset[i] = pos;
    std::uint32_t n = 0;
    forEachTap(i, [&](int, int, double) {
      ++n;
    });
    pos += n;
    if (n < c.minPerLed)
      c.minPerLed = n;
  }
  c.offset[kLedCount] = pos;
  c.total = pos;
  return c;
}
inline constexpr Counts kCounts = countTaps();
static_assert(kCounts.minPerLed >= 1, "Every LED needs at least one tap: increase the footprint");
static_assert(kCounts.total < 65536, "Tap offsets are stored on 16 bits");

// Pass 2: fill indices and normalised Q0.16 weights.
template<std::size_t T> struct TapTable
{
  std::array<std::uint16_t, kLedCount + 1> offset {};
  std::array<Tap, T> taps {};
  std::array<std::uint8_t, T> peak {}; // weight relative to the LED's strongest tap, Q0.8 (255 = strongest)
  bool weightOverflow = false;
};

constexpr TapTable<kCounts.total> buildTaps()
{
  TapTable<kCounts.total> t {};
  std::uint32_t pos = 0;
  for (std::size_t i = 0; i < kLedCount; ++i)
  {
    t.offset[i] = static_cast<std::uint16_t>(pos);

    double sum = 0.0;
    forEachTap(i, [&](int, int, double d2) {
      sum += gauss(d2);
    });

    const std::uint32_t first = pos;
    std::uint32_t best = pos;
    std::uint32_t bestW = 0;
    std::int32_t qsum = 0;
    forEachTap(i, [&](int x, int y, double d2) {
      std::uint32_t q = static_cast<std::uint32_t>(gauss(d2) / sum * 65536.0 + 0.5);
      if (q > 65535u)
      {
        q = 65535u;
        t.weightOverflow = true;
      }
      t.taps[pos] = Tap {static_cast<std::uint8_t>(x), static_cast<std::uint8_t>(y), static_cast<std::uint16_t>(q)};
      qsum += static_cast<std::int32_t>(q);
      if (q > bestW)
      {
        bestW = q;
        best = pos;
      }
      ++pos;
    });
    // Make the sum exactly 65536 (a uniform canvas stays uniform), residual goes on the largest tap.
    const std::int32_t fixed = static_cast<std::int32_t>(bestW) + (65536 - qsum);
    if (fixed > 65535 || fixed < 0)
      t.weightOverflow = true;
    if (pos > first)
    {
      t.taps[best].w = static_cast<std::uint16_t>(fixed);
      // Peak weights: strongest tap = peakStrength * 255, others proportional.
      const double wmax = static_cast<double>(fixed);
      for (std::uint32_t k = first; k < pos; ++k)
      {
        const double rel = static_cast<double>(t.taps[k].w) / wmax;
        double p = rel * 255.0 * config::peakStrength + 0.5;
        if (p > 255.0)
          p = 255.0;
        t.peak[k] = static_cast<std::uint8_t>(p);
      }
    }
  }
  t.offset[kLedCount] = static_cast<std::uint16_t>(pos);
  return t;
}
} // namespace detail

inline constexpr auto kTaps = detail::buildTaps();
static_assert(!kTaps.weightOverflow, "Weight does not fit in Q0.16: widen the footprint");

// ================================ Canvas ===================================
// Canvas of packed 16 bit pixels. x wraps around the cylinder, y is clipped.

template<int Width = kCanvasWidth, int Height = kCanvasHeight> class Canvas
{
public:
  // constructors
  Canvas() = default;

  Canvas(const Canvas&) = delete;
  Canvas& operator=(const Canvas&) = delete;

  static constexpr int width = Width;
  static constexpr int height = Height;

  /// Fill the whole canvas black
  void clear() { fill(Rgb8 {}); }

  /// Fill the canvas with a target color
  void fill(Rgb8 c) { fillRaw(Codec::pack(c)); }

  /// Assign the pixel at position a new color
  void setPixel(int x, int y, Rgb8 c) { setRaw(x, y, Codec::pack(c)); }
  void setPixel(int x, int y, uint32_t c) { setPixel(x, y, Rgb8::from_uint32(c)); }

  /// Get the color at a target position
  Rgb8 getPixel(int x, int y) const { return Codec::unpack(getRaw(x, y)); }

  /**
   * \brief Render this canvas on the output
   */
  void render(LampTy& lamp, int shiftX = 0) const
  {
    const int s = Canvas::wrapX(shiftX);

    for (std::size_t i = 0; i < kLedCount; ++i)
    {
      std::uint32_t r = 32768u, g = 32768u, b = 32768u; // +0.5 for rounding; max 255 * 65536 fits in 32 bits
      std::uint32_t pr = 0, pg = 0, pb = 0;             // peak terms, max 255 * 255
      for (std::uint32_t t = kTaps.offset[i]; t < kTaps.offset[i + 1]; ++t)
      {
        const detail::Tap& tap = kTaps.taps[t];
        int x = tap.x + s;
        if (x >= Canvas::width)
          x -= Canvas::width;
        const Rgb8 p = Codec::unpack(px_[std::size_t(tap.y) * Canvas::width + x]);
        r += std::uint32_t(tap.w) * p.r;
        g += std::uint32_t(tap.w) * p.g;
        b += std::uint32_t(tap.w) * p.b;

        const std::uint32_t wp = kTaps.peak[t];
        const std::uint32_t qr = wp * p.r, qg = wp * p.g, qb = wp * p.b;
        if (qr > pr)
          pr = qr;
        if (qg > pg)
          pg = qg;
        if (qb > pb)
          pb = qb;
      }
      const std::uint32_t ar = r >> 16, ag = g >> 16, ab = b >> 16;
      const std::uint32_t kr = (pr + 127u) / 255u, kg = (pg + 127u) / 255u, kb = (pb + 127u) / 255u;
      Rgb8 color {static_cast<std::uint8_t>(ar > kr ? ar : kr),
                  static_cast<std::uint8_t>(ag > kg ? ag : kg),
                  static_cast<std::uint8_t>(ab > kb ? ab : kb)};
      lamp.setPixelColor(i, color.to_uint32());
    }
  }

  static constexpr int wrapX(int x)
  {
    x %= Width;
    return x < 0 ? x + Width : x;
  }

protected:
  /**
   * \brief Set a pixel to the expected internal color format directly
   */
  void setRaw(int x, int y, std::uint16_t v)
  {
    if (y < 0 || y >= Height)
      return;
    px_[std::size_t(y) * Width + wrapX(x)] = v;
  }

  /**
   * \brief Get a pixel in the expected internal color format directly
   */
  std::uint16_t getRaw(int x, int y) const
  {
    if (y < 0 || y >= Height)
      return 0;
    return px_[std::size_t(y) * Width + wrapX(x)];
  }

  /**
   * \brief Fill with an internal color codec directly
   */
  void fillRaw(std::uint16_t v)
  {
    for (auto& p: px_)
      p = v;
  }

private:
  std::array<std::uint16_t, std::size_t(Width) * Height> px_ {};
};

} // namespace canvas
} // namespace lampda::modes::draw

#ifndef UTILS_CURVES_H
#define UTILS_CURVES_H

#include "src/system/utils/utils.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace curves {

template<typename T, typename U> struct Point
{
  T x;
  U y;
};

/**
 * Given a set of points, will fit multiple linear segments to it.
 *
 */
template<typename T, typename U> class LinearCurve
{
public:
  using point_t = Point<T, U>;

  LinearCurve(const std::vector<point_t>& points)
  {
    pts = points;

    // sort the vector by the x coordinate
    auto lbd = [](const point_t& a, const point_t& b) {
      return a.x < b.x;
    };
    std::sort(pts.begin(), pts.end(), lbd);
  }

  U sample(const T x) const
  {
    // error
    if (pts.size() < 2)
      return 0;

    point_t lastPt = pts[0];
    // low bound failure
    if (x < lastPt.x)
    {
      return lastPt.y;
    }

    for (size_t i = 1; i < pts.size(); ++i)
    {
      const point_t& pt = pts[i];
      // in this segment bound
      if (x >= lastPt.x and x <= pt.x)
      {
        return lmpd_map<T, U>(x, lastPt.x, pt.x, lastPt.y, pt.y);
      }
      // update last point
      lastPt = pt;
    }

    // highest bound failure
    return lastPt.y;
  }

private:
  std::vector<point_t> pts;
};

/**
 * Given two points and an exponent, fit an exponential function
 *
 */
template<typename T, typename U> class ExponentialCurve
{
public:
  using point_t = Point<T, U>;

  /**
   * \brief Exponential curve fitting to two points
   * \param[in] pointA
   * \param[in] pointB
   * \param[in] exponent The strenght factor of this exponential. Set to 1 for a linear curve, > 1 gives strong
   * exponential, < 1  gives log curves
   */
  ExponentialCurve(const point_t& pointA, const point_t& pointB, const double exponent = 15.0) :
    lowerBound(pointA.y),
    upperBound(pointB.y)
  {
    const double div = pointA.y / static_cast<double>(pointB.y);
    if (div <= -1.0)
    {
      // Error: linear approx...
      _exp = 1;
      _a = 0;
      _b = 1;
      return;
    }
    const double A = exp(log(div) / exponent);
    _exp = exponent;
    _a = (static_cast<double>(pointA.x) - static_cast<double>(pointB.x) * A) / (A - 1);
    _b = static_cast<double>(pointA.y) / pow(static_cast<double>(pointA.x + _a), exponent);
  }

  /**
   * \brief Sample the exponential curve
   * Be aware that the result is always casted from a floating point !!
   * -> for integer types, it will be the same as calling "floor()" on the result
   */
  float sample(const T x) const
  {
    const float res = pow(static_cast<double>(x) + _a, _exp) * _b;
    return lmpd_constrain(res, lowerBound, upperBound);
  }

private:
  const T lowerBound;
  const T upperBound;

  double _exp;
  double _a;
  double _b;
};

} // namespace curves

#endif

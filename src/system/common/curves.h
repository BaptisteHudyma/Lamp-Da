/*! \file curves.h
    \brief Define curves types, that can be sampled.
*/

#pragma once

#include "src/system/utils/utils.h"

#include <cassert>
#include <cmath>
#include <array>

namespace lampda {
namespace common {
/// Curves classes to sample values from predefined custom parameters.
namespace curves {

/**
 * \brief Define a 2D point
 * \param[in] T Type of X coordinates
 * \param[in] U type of Y coordinates
 */
template<typename T, typename U> struct Point
{
  T x; ///< X coordinate of the point
  U y; ///< Y coordinate of the point
};

/**
 * \brief Given a set of points, will fit multiple linear segments to it.
 * \param[in] T Type of X coordinates
 * \param[in] U type of Y coordinates
 * \param[in] maxSize max size of the given array (deduced)
 */
template<typename T, typename U, size_t maxSize = 24> class LinearCurve
{
public:
  /// point in the linear curve
  using point_t = Point<T, U>;

  /// Build a curve from a set of points
  template<size_t N> LinearCurve(const std::array<point_t, N>& points) : _count(0)
  {
    static_assert(N <= maxSize, "Array size must be less than MaxSize");
    // first fail fast
    assert(points.size() >= 2 && "Linear curve must have more than 1 points");

    // Copy points
    for (size_t i = 0; i < N; ++i)
    {
      pts[i] = points[i];
    }

    // Validate all points
    for (size_t i = 0; i < N; ++i)
    {
      const auto& p = pts[i];
      assert(not std::isnan(p.x) && "invalid value in curve parameters");
      assert(std::isfinite(p.x) && "invalid value in curve parameters");
      assert(not std::isnan(p.y) && "invalid value in curve parameters");
      assert(std::isfinite(p.y) && "invalid value in curve parameters");
    }

    // Verify points are sorted by X coordinate
    for (size_t i = 1; i < N; ++i)
    {
      assert(pts[i - 1].x < pts[i].x && "Points must be sorted by X coordinate in ascending order");
    }

    // Remove consecutive duplicates (in place)
    size_t writeIdx = 1;
    for (size_t readIdx = 1; readIdx < N; ++readIdx)
    {
      if (pts[readIdx - 1].x != pts[readIdx].x || pts[readIdx - 1].y != pts[readIdx].y)
      {
        pts[writeIdx] = pts[readIdx];
        writeIdx++;
      }
    }
    _count = writeIdx;

    assert(_count >= 2 && "Linear curve must have more than 1 points");
  }

  /// Sample a point Y from a given x
  U sample(const T x) const
  {
    point_t lastPt = pts[0];
    // bounds failure
    if (std::isnan(x))
      return lastPt.y;
    if (x <= lastPt.x)
      return lastPt.y;
    if (x >= pts[_count - 1].x)
      return pts[_count - 1].y;

    for (size_t i = 1; i < _count; ++i)
    {
      const point_t& pt = pts[i];
      // in this segment bound
      if (x >= lastPt.x and x <= pt.x)
      {
        return lmpd_map<U>(x, lastPt.x, pt.x, lastPt.y, pt.y);
      }
      // update last point
      lastPt = pt;
    }

    // highest bound failure
    return lastPt.y;
  }

private:
  /// linear end points of the linear segments
  std::array<point_t, maxSize> pts;
  /// number of valid points in the array
  size_t _count;
};

template<typename T, typename U, typename... Args>
LinearCurve<T, U> make_linear_curve(const Point<T, U>& firstPoint, const Point<T, U>& secondPoint, Args&&... args)
{
  constexpr size_t N = 2 + sizeof...(Args);
  std::array<Point<T, U>, N> pts = {{firstPoint, secondPoint, args...}};
  return LinearCurve<T, U>(pts);
}

/**
 * \brief Given two points and an exponent, fit an exponential function
 * \param[in] T Type of X coordinates
 * \param[in] U type of Y coordinates
 */
template<typename T, typename U> class ExponentialCurve
{
public:
  /// point in the exponential curve
  using point_t = Point<T, U>;

  /**
   * \brief Exponential curve fitting to two points
   * \param[in] pointA Start point of the curve
   * \param[in] pointB End point of the curve
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
   * \brief Sample the exponential curve.
   * Be aware that the result is always casted from a floating point !!
   * -> for integer types, it will be the same as calling "floor()" on the result
   */
  float sample(const T x) const
  {
    const float res = pow(static_cast<double>(x) + _a, _exp) * _b;
    return lmpd_constrain<float>(res, lowerBound, upperBound);
  }

private:
  /// lower bound of the X coordinates
  const T lowerBound;
  /// upper bound of the X coordinates
  const T upperBound;

  /// exponent parameters
  double _exp;
  /// exponent added parameters
  double _a;
  /// exponent multiplied parameters
  double _b;
};

} // namespace curves
} // namespace common
} // namespace lampda

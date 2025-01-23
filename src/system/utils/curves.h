#ifndef UTILS_CURVES_H
#define UTILS_CURVES_H

#include "src/system/utils/utils.h"

#include <algorithm>
#include <vector>

template<typename T, typename U> class Curve
{
public:
  struct Point
  {
    T x;
    U y;
  };

  Curve(const std::vector<Point>& points)
  {
    pts = points;

    // sort the vector by the x coordinate
    auto lbd = [](const Point& a, const Point& b) {
      return a.x < b.x;
    };
    std::sort(pts.begin(), pts.end(), lbd);
  }

  U sample(const T x) const
  {
    // error
    if (pts.size() < 2)
      return 0;

    Point lastPt = pts[0];
    // low bound failure
    if (x < lastPt.x)
    {
      return lastPt.y;
    }

    for (size_t i = 1; i < pts.size(); ++i)
    {
      Point pt = pts[i];
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
  std::vector<Point> pts;
};

#endif
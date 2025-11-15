#include "gtest/gtest.h"
#include <gtest/gtest.h>
#include "src/system/utils/curves.h"

static constexpr float Inf = std::numeric_limits<float>::infinity();

TEST(test_curves, invalid_linear_curve_create)
{
  using Curve = curves::LinearCurve<float, float>;
  std::vector<Curve::point_t> points;
  // zero point
  ASSERT_DEATH({ const Curve curve(points); }, ".*Linear curve must have more than 1 points.*");

  // one point
  points = {Curve::point_t {0.0f, 0.0f}};
  ASSERT_DEATH({ const Curve curve(points); }, ".*Linear curve must have more than 1 points.*");

  // two or more identical points
  points = {Curve::point_t {0.0f, 0.0f}, Curve::point_t {0.0f, 0.0f}};
  ASSERT_DEATH({ const Curve curve(points); }, ".*Linear curve must have more than 1 points.*");
  points = {Curve::point_t {1000.0f, 0.0f}, Curve::point_t {1000.0f, 0.0f}, Curve::point_t {1000.0f, 0.0f}};
  ASSERT_DEATH({ const Curve curve(points); }, ".*Linear curve must have more than 1 points.*");

  // add invalid points
  points = {Curve::point_t {NAN, 0.0f}, Curve::point_t {1.0f, 1.0f}};
  ASSERT_DEATH({ const Curve curve(points); }, ".*invalid value in curve parameters.*");
  points = {Curve::point_t {0.0f, NAN}, Curve::point_t {1.0f, 1.0f}};
  ASSERT_DEATH({ const Curve curve(points); }, ".*invalid value in curve parameters.*");
  points = {Curve::point_t {0.0f, 0.0f}, Curve::point_t {NAN, 1.0f}};
  ASSERT_DEATH({ const Curve curve(points); }, ".*invalid value in curve parameters.*");
  points = {Curve::point_t {0.0f, 0.0f}, Curve::point_t {1.0f, NAN}};
  ASSERT_DEATH({ const Curve curve(points); }, ".*invalid value in curve parameters.*");
  points = {Curve::point_t {Inf, 0.0f}, Curve::point_t {1.0f, 1.0f}};
  ASSERT_DEATH({ const Curve curve(points); }, ".*invalid value in curve parameters.*");
  points = {Curve::point_t {0.0f, Inf}, Curve::point_t {1.0f, 1.0f}};
  ASSERT_DEATH({ const Curve curve(points); }, ".*invalid value in curve parameters.*");
  points = {Curve::point_t {0.0f, 0.0f}, Curve::point_t {Inf, 1.0f}};
  ASSERT_DEATH({ const Curve curve(points); }, ".*invalid value in curve parameters.*");
  points = {Curve::point_t {0.0f, 0.0f}, Curve::point_t {1.0f, Inf}};
  ASSERT_DEATH({ const Curve curve(points); }, ".*invalid value in curve parameters.*");
}

TEST(test_curves, two_points_linear_float_curve)
{
  using Curve = curves::LinearCurve<float, float>;
  std::vector<Curve::point_t> points {Curve::point_t {0.0f, 0.0f}, Curve::point_t {100.0f, 100.0f}};
  Curve curve(points);

  for (float i = 0.0f; i < 100.0f; i += 1.0f)
  {
    const auto res = curve.sample(i);
    ASSERT_EQ(res, i);
    ASSERT_EQ(typeid(res), typeid(float));
  }
  // min born constraint
  auto res = curve.sample(-100.0f);
  ASSERT_EQ(res, 0.0f);
  ASSERT_EQ(typeid(res), typeid(float));
  // max born constraint
  res = curve.sample(1000.0f);
  ASSERT_EQ(res, 100.0f);
  ASSERT_EQ(typeid(res), typeid(float));
  // weird values
  res = curve.sample(-Inf);
  ASSERT_EQ(res, 0.0f);
  ASSERT_EQ(typeid(res), typeid(float));
  res = curve.sample(Inf);
  ASSERT_EQ(res, 100.0f);
  ASSERT_EQ(typeid(res), typeid(float));
  res = curve.sample(NAN);
  ASSERT_EQ(res, 0.0f);
  ASSERT_EQ(typeid(res), typeid(float));

  /**
   * inverted linear curve
   */

  points = {Curve::point_t {0.0f, 100.0f}, Curve::point_t {100.0f, 0.0f}};
  curve = Curve(points);
  for (float i = 0.0f; i < 100.0f; i += 1.0f)
  {
    const auto res = curve.sample(i);
    ASSERT_EQ(res, 100.0f - i);
    ASSERT_EQ(typeid(res), typeid(float));
  }
  // min born constraint
  res = curve.sample(-100.0f);
  ASSERT_EQ(res, 100.0f);
  ASSERT_EQ(typeid(res), typeid(float));
  // max born constraint
  res = curve.sample(1000.0f);
  ASSERT_EQ(res, 0.0f);
  ASSERT_EQ(typeid(res), typeid(float));
  // weird values
  res = curve.sample(-Inf);
  ASSERT_EQ(res, 100.0f);
  ASSERT_EQ(typeid(res), typeid(float));
  res = curve.sample(Inf);
  ASSERT_EQ(res, 0.0f);
  ASSERT_EQ(typeid(res), typeid(float));
  res = curve.sample(NAN);
  ASSERT_EQ(res, 100.0f);
  ASSERT_EQ(typeid(res), typeid(float));

  /**
   * negative inverted linear curve
   */

  points = {Curve::point_t {0.0f, -100.0f}, Curve::point_t {100.0f, 0.0f}};
  curve = Curve(points);
  for (float i = 0.0f; i < 100.0f; i += 1.0f)
  {
    const auto res = curve.sample(i);
    ASSERT_EQ(res, i - 100.0f);
    ASSERT_EQ(typeid(res), typeid(float));
  }
  // min born constraint
  res = curve.sample(-100.0f);
  ASSERT_EQ(res, -100.0f);
  ASSERT_EQ(typeid(res), typeid(float));
  // max born constraint
  res = curve.sample(1000.0f);
  ASSERT_EQ(res, 0.0f);
  ASSERT_EQ(typeid(res), typeid(float));
  // weird values
  res = curve.sample(-Inf);
  ASSERT_EQ(res, -100.0f);
  ASSERT_EQ(typeid(res), typeid(float));
  res = curve.sample(Inf);
  ASSERT_EQ(res, 0.0f);
  ASSERT_EQ(typeid(res), typeid(float));
  res = curve.sample(NAN);
  ASSERT_EQ(res, -100.0f);
  ASSERT_EQ(typeid(res), typeid(float));

  /**
   * negative inverted linear curve, 3 points
   */

  points = {Curve::point_t {-100.0f, -100.0f}, Curve::point_t {0.0f, 0.0f}, Curve::point_t {100.0f, 100.0f}};
  curve = Curve(points);
  for (float i = -100.0f; i < 100.0f; i += 1.0f)
  {
    const auto res = curve.sample(i);
    ASSERT_EQ(res, i);
    ASSERT_EQ(typeid(res), typeid(float));
  }
  // min born constraint
  res = curve.sample(-1000.0f);
  ASSERT_EQ(res, -100.0f);
  ASSERT_EQ(typeid(res), typeid(float));
  // max born constraint
  res = curve.sample(1000.0f);
  ASSERT_EQ(res, 100.0f);
  ASSERT_EQ(typeid(res), typeid(float));
  // weird values
  res = curve.sample(-Inf);
  ASSERT_EQ(res, -100.0f);
  ASSERT_EQ(typeid(res), typeid(float));
  res = curve.sample(Inf);
  ASSERT_EQ(res, 100.0f);
  ASSERT_EQ(typeid(res), typeid(float));
  res = curve.sample(NAN);
  ASSERT_EQ(res, -100.0f);
  ASSERT_EQ(typeid(res), typeid(float));

  /**
   * low slope curve
   */

  points = {Curve::point_t {0.0f, 0.0f}, Curve::point_t {100.0f, 1.0f}};
  curve = Curve(points);

  ASSERT_EQ(curve.sample(0.0f), 0.0f);
  ASSERT_EQ(curve.sample(50.0f), 0.5f);
  ASSERT_EQ(curve.sample(100.0f), 1.0f);
}

TEST(test_curves, N_points_linear_curve)
{
  using CurveFloatFloat = curves::LinearCurve<float, float>;
  // unsorted linear curve
  std::vector<CurveFloatFloat::point_t> pointsFF {CurveFloatFloat::point_t {300.0f, 300.0f},
                                                  CurveFloatFloat::point_t {-100.0f, -100.0f},
                                                  CurveFloatFloat::point_t {200.0f, 200.0f},
                                                  CurveFloatFloat::point_t {100.0f, 100.0f}};
  CurveFloatFloat curveFF(pointsFF);
  ASSERT_EQ(curveFF.sample(0.0f), 0.0f);
  ASSERT_EQ(curveFF.sample(100.0f), 100.0f);
  ASSERT_EQ(curveFF.sample(-100.0f), -100.0f);

  using CurveFloatUint = curves::LinearCurve<float, uint8_t>;
  // unsorted linear curve
  std::vector<CurveFloatUint::point_t> pointsFU {CurveFloatUint::point_t {300.0f, 255},
                                                 CurveFloatUint::point_t {-100.0f, 0},
                                                 CurveFloatUint::point_t {200.0f, 191},
                                                 CurveFloatUint::point_t {100.0f, 127}};
  CurveFloatUint curveFU(pointsFU);
  ASSERT_EQ(curveFU.sample(-100.0f), 0);
  ASSERT_EQ(curveFU.sample(-50.0f), 31);
  ASSERT_EQ(curveFU.sample(0.0f), 63);
  ASSERT_EQ(curveFU.sample(50.0f), 95);
  ASSERT_EQ(curveFU.sample(100.0f), 127);
  ASSERT_EQ(curveFU.sample(150.0f), 159);
  ASSERT_EQ(curveFU.sample(200.0f), 191);
  ASSERT_EQ(curveFU.sample(250.0f), 223);
  ASSERT_EQ(curveFU.sample(300.0f), 255);
  // invalid vals
  ASSERT_EQ(curveFU.sample(NAN), 0);
  ASSERT_EQ(curveFU.sample(Inf), 255);
  ASSERT_EQ(curveFU.sample(-Inf), 0);
}

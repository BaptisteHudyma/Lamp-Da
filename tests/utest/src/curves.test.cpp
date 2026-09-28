#include "gtest/gtest.h"
#include <gtest/gtest.h>

#include "src/system/common/curves.h"

namespace lampda::common {

static constexpr float Inf = std::numeric_limits<float>::infinity();

TEST(test_curves, invalid_linear_curve_create)
{
  using Curve = curves::LinearCurve<float, float>;

  // two or more identical points
  ASSERT_DEATH(
          {
            const Curve curve = curves::make_linear_curve(Curve::point_t {0.0f, 0.0f}, Curve::point_t {0.0f, 0.0f});
          },
          ".*Linear curve must have more than 1 unique points.*");
  ASSERT_DEATH(
          {
            const Curve curve = curves::make_linear_curve(
                    Curve::point_t {1000.0f, 0.0f}, Curve::point_t {1000.0f, 0.0f}, Curve::point_t {1000.0f, 0.0f});
          },
          ".*Linear curve must have more than 1 unique points.*");

  // add invalid points
  ASSERT_DEATH(
          {
            const Curve curve = curves::make_linear_curve(Curve::point_t {NAN, 0.0f}, Curve::point_t {1.0f, 1.0f});
          },
          ".*invalid value in curve parameters.*");
  ASSERT_DEATH(
          {
            const Curve curve = curves::make_linear_curve(Curve::point_t {0.0f, NAN}, Curve::point_t {1.0f, 1.0f});
          },
          ".*invalid value in curve parameters.*");
  ASSERT_DEATH(
          {
            const Curve curve = curves::make_linear_curve(Curve::point_t {0.0f, 0.0f}, Curve::point_t {NAN, 1.0f});
          },
          ".*invalid value in curve parameters.*");
  ASSERT_DEATH(
          {
            const Curve curve = curves::make_linear_curve(Curve::point_t {0.0f, 0.0f}, Curve::point_t {1.0f, NAN});
          },
          ".*invalid value in curve parameters.*");
  ASSERT_DEATH(
          {
            const Curve curve = curves::make_linear_curve(Curve::point_t {Inf, 0.0f}, Curve::point_t {1.0f, 1.0f});
          },
          ".*invalid value in curve parameters.*");
  ASSERT_DEATH(
          {
            const Curve curve = curves::make_linear_curve(Curve::point_t {0.0f, Inf}, Curve::point_t {1.0f, 1.0f});
          },
          ".*invalid value in curve parameters.*");
  ASSERT_DEATH(
          {
            const Curve curve = curves::make_linear_curve(Curve::point_t {0.0f, 0.0f}, Curve::point_t {Inf, 1.0f});
          },
          ".*invalid value in curve parameters.*");
  ASSERT_DEATH(
          {
            const Curve curve = curves::make_linear_curve(Curve::point_t {0.0f, 0.0f}, Curve::point_t {1.0f, Inf});
          },
          ".*invalid value in curve parameters.*");
}

TEST(test_curves, two_points_linear_float_curve)
{
  using Curve = curves::LinearCurve<float, float>;
  Curve curve = curves::make_linear_curve(Curve::point_t {0.0f, 0.0f}, Curve::point_t {100.0f, 100.0f});

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

  curve = curves::make_linear_curve(Curve::point_t {0.0f, 100.0f}, Curve::point_t {100.0f, 0.0f});
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

  curve = curves::make_linear_curve(Curve::point_t {0.0f, -100.0f}, Curve::point_t {100.0f, 0.0f});
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

  curve = curves::make_linear_curve(
          Curve::point_t {-100.0f, -100.0f}, Curve::point_t {0.0f, 0.0f}, Curve::point_t {100.0f, 100.0f});
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

  curve = curves::make_linear_curve(Curve::point_t {0.0f, 0.0f}, Curve::point_t {100.0f, 1.0f});

  ASSERT_EQ(curve.sample(0.0f), 0.0f);
  ASSERT_EQ(curve.sample(50.0f), 0.5f);
  ASSERT_EQ(curve.sample(100.0f), 1.0f);
}

TEST(test_curves, N_points_linear_curve)
{
  using CurveFloatFloat = curves::LinearCurve<float, float>;
  // unsorted linear curve
  CurveFloatFloat curveFF = curves::make_linear_curve(CurveFloatFloat::point_t {-100.0f, -100.0f},
                                                      CurveFloatFloat::point_t {100.0f, 100.0f},
                                                      CurveFloatFloat::point_t {200.0f, 200.0f},
                                                      CurveFloatFloat::point_t {300.0f, 300.0f});
  ASSERT_EQ(curveFF.sample(0.0f), 0.0f);
  ASSERT_EQ(curveFF.sample(100.0f), 100.0f);
  ASSERT_EQ(curveFF.sample(-100.0f), -100.0f);

  using CurveFloatUint = curves::LinearCurve<float, uint8_t>;
  // unsorted linear curve
  CurveFloatUint curveFU = curves::make_linear_curve(CurveFloatUint::point_t {-100.0f, 0},
                                                     CurveFloatUint::point_t {100.0f, 127},
                                                     CurveFloatUint::point_t {200.0f, 191},
                                                     CurveFloatUint::point_t {300.0f, 255});
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

TEST(test_curves, N_points_linear_curve_unsorted)
{
  using CurveFloatFloat = curves::LinearCurve<float, float>;
  // unsorted linear curve
  ASSERT_DEATH(
          {
            CurveFloatFloat curveFF = curves::make_linear_curve(CurveFloatFloat::point_t {300.0f, 300.0f},
                                                                CurveFloatFloat::point_t {-100.0f, -100.0f},
                                                                CurveFloatFloat::point_t {200.0f, 200.0f},
                                                                CurveFloatFloat::point_t {100.0f, 100.0f});
          },
          ".*Points must be sorted by X coordinate in ascending order.*");

  // unsorted linear curve
  using CurveFloatUint = curves::LinearCurve<float, uint8_t>;
  ASSERT_DEATH(
          {
            CurveFloatUint curveFU = curves::make_linear_curve(CurveFloatUint::point_t {300.0f, 255},
                                                               CurveFloatUint::point_t {-100.0f, 0},
                                                               CurveFloatUint::point_t {200.0f, 191},
                                                               CurveFloatUint::point_t {100.0f, 127});
          },
          ".*Points must be sorted by X coordinate in ascending order.*");
}

} // namespace lampda::common

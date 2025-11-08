#include <cmath>
#include <gtest/gtest.h>
#include <limits>
#include "src/system/utils/utils.h"

static constexpr float Inf = std::numeric_limits<float>::infinity();

/**
 * Test min
 */

TEST(test_min_max, invalid_values)
{
  ASSERT_DEATH({ min(NAN, 0.0f); }, ".*invalid param a.*");
  ASSERT_DEATH({ max(NAN, 0.0f); }, ".*invalid param a.*");

  ASSERT_DEATH({ min(0.0f, NAN); }, ".*invalid param b.*");
  ASSERT_DEATH({ max(0.0f, NAN); }, ".*invalid param b.*");

  ASSERT_EQ(min(Inf, 0.0f), 0.0f);
  ASSERT_EQ(min(-Inf, 0.0f), -Inf);

  ASSERT_EQ(max(Inf, 0.0f), Inf);
  ASSERT_EQ(max(-Inf, 0.0f), 0.0);
}

TEST(test_min_max, normal_use)
{
  for (size_t i = 1; i < 300; i++)
  {
    ASSERT_EQ(min(i, 0), 0);
    ASSERT_EQ(min(0, i), 0);

    ASSERT_EQ(max(0, i), i);
    ASSERT_EQ(max(i, 0), i);
  }
}

/**
 * lmpd_constrain tests
 */

TEST(test_constrain, invalidBorns)
{
  ASSERT_DEATH(
          {
            const int minA = -1000;
            const int maxA = 1000;
            const auto resInt = lmpd_constrain(15.0, maxA, minA);
          },
          "Assertion `mini < maxi' failed");

  ASSERT_DEATH(
          {
            const int minA = 1000;
            const int maxA = 1000;
            const auto resInt = lmpd_constrain(15.0, maxA, minA);
          },
          "Assertion `mini < maxi' failed");
}

TEST(test_constrain, same_type)
{
  const int minA = -1000;
  const int maxA = 1000;
  int a;
  for (a = minA; a < maxA; a++)
  {
    const auto resInt = lmpd_constrain(a, minA, maxA);
    ASSERT_EQ(resInt, a);
    ASSERT_EQ(typeid(resInt), typeid(a));
  }
}

TEST(test_constrain, negative_size)
{
  const int minA = -1000;
  const int maxA = 1000;

  for (size_t a = 0; a < maxA; a++)
  {
    const auto res = lmpd_constrain(a, minA, maxA);
    ASSERT_EQ(res, a);
    ASSERT_EQ(typeid(res), typeid(a));
  }
}

TEST(test_constrain, invalidvals)
{
  // not a number
  const auto resNan = lmpd_constrain(NAN, -100.0f, 100.0f);
  ASSERT_EQ(resNan, -100.0f);

  // infinity
  const auto resInf = lmpd_constrain(Inf, -100.0f, 100.0f);
  ASSERT_EQ(resInf, 100.0f);
  const auto resMInf = lmpd_constrain(-Inf, -100.0f, 100.0f);
  ASSERT_EQ(resMInf, -100.0f);

  const auto resMinInf = lmpd_constrain(-1000000.0f, -Inf, 100.0f);
  ASSERT_EQ(resMinInf, -1000000.0f);
  const auto resMaxInf = lmpd_constrain(1000000.0f, 100.0f, Inf);
  ASSERT_EQ(resMaxInf, 1000000.0f);
}

/**
 * lmpd_map tests
 */

TEST(test_map, valid)
{
  const int minA = -1000;
  const int maxA = 1000;
  for (int a = minA; a < maxA; a++)
  {
    const auto resA = lmpd_map(a, minA, maxA, minA, maxA);
    ASSERT_EQ(resA, a);

    const auto resB = lmpd_map(a, minA, maxA, maxA, minA);
    ASSERT_EQ(resB, -a);
  }

  const float minB = -1000.0f;
  const float maxB = 1000.0f;
  for (float a = minB; a < maxB; a += 0.5f)
  {
    const auto resA = lmpd_map(a, minB, maxB, minB, maxB);
    ASSERT_EQ(resA, a);

    const auto resB = lmpd_map(a, minB, maxB, maxB, minB);
    ASSERT_EQ(resB, -a);
  }
}

TEST(test_map, positive_to_negative)
{
  size_t i = 0;
  size_t minA = 0;
  size_t minB = 100;
  const auto resA = lmpd_map(i, minA, minB, -1.0f, 1.0f);
  ASSERT_EQ(resA, -1.0f);
  ASSERT_EQ(typeid(resA), typeid(float));
}

TEST(test_map, invalid_borns)
{
  ASSERT_EQ(lmpd_map(NAN, 0.0f, 1.0f, 0.0f, 1.0f), 0.0f);
  ASSERT_EQ(lmpd_map(NAN, 0.0f, 1.0f, 0.0f, 1.0f), 0.0f);

  ASSERT_EQ(lmpd_map(Inf, 0.0f, 1.0f, 0.0f, 1.0f), 1.0f);
  ASSERT_EQ(lmpd_map(-Inf, 0.0f, 1.0f, 0.0f, 1.0f), 0.0f);

  ASSERT_DEATH({ lmpd_map(0.0f, NAN, 1.0f, 0.0f, 1.0f); }, ".*in_min invalid.*");
  ASSERT_DEATH({ lmpd_map(0.0f, Inf, 1.0f, 0.0f, 1.0f); }, ".*in_min invalid.*");

  ASSERT_DEATH({ lmpd_map(0.0f, 0.0f, NAN, 0.0f, 1.0f); }, ".*in_max invalid.*");
  ASSERT_DEATH({ lmpd_map(0.0f, 0.0f, Inf, 0.0f, 1.0f); }, ".*in_max invalid.*");

  ASSERT_DEATH({ lmpd_map(0.0f, 0.0f, 1.0f, NAN, 1.0f); }, ".*out_min invalid.*");
  ASSERT_DEATH({ lmpd_map(0.0f, 0.0f, 1.0f, Inf, 1.0f); }, ".*out_min invalid.*");

  ASSERT_DEATH({ lmpd_map(0.0f, 0.0f, 1.0f, 0.0f, NAN); }, ".*out_max invalid.*");
  ASSERT_DEATH({ lmpd_map(0.0f, 0.0f, 1.0f, 0.0f, Inf); }, ".*out_max invalid.*");
}

TEST(test_map, type_change)
{
  int minA = -200;
  int maxA = 200;
  for (int i = 0; i < maxA; i++)
  {
    const auto resA = lmpd_map(0.0f, 0.0f, 1.0f, i, maxA);
    ASSERT_EQ(resA, i);
    ASSERT_EQ(typeid(resA), typeid(i));
    //
    const auto resB = lmpd_map(1.0f, 0.0f, 1.0f, minA, i);
    ASSERT_EQ(resB, i);
    ASSERT_EQ(typeid(resB), typeid(i));
  }
}

/**
 * Test angles
 */

TEST(test_angles, to_radians)
{
  ASSERT_EQ(to_radians(0), 0);
  ASSERT_EQ(to_radians(90), M_PIf * 1.0f / 2.0f);
  ASSERT_EQ(to_radians(180), M_PIf * 2.0f / 2.0f);
  ASSERT_EQ(to_radians(270), M_PIf * 3.0f / 2.0f);
  ASSERT_EQ(to_radians(360), M_PIf * 4.0f / 2.0f);
}

TEST(test_angles, invalid_to_radians)
{
  ASSERT_DEATH({ to_radians(NAN); }, ".*invalid value.*");
  ASSERT_DEATH({ to_radians(Inf); }, ".*invalid value.*");
}

TEST(test_angles, wrap)
{
  ASSERT_EQ(wrap_angle(0.0f), 0.0f);
  ASSERT_EQ(wrap_angle(M_PIf * 0.5f), M_PIf * 0.5f);
  ASSERT_EQ(wrap_angle(M_PIf), M_PIf);

  ASSERT_EQ(wrap_angle(M_PIf * 2), 0.0f);
  ASSERT_EQ(wrap_angle(-M_PIf * 2), 0.0f);
}

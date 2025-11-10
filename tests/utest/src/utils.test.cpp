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
  int minVal = -100;
  int maxVal = -100;
  for (int i = minVal; i < maxVal; i++)
  {
    ASSERT_EQ(min(i, minVal), minVal);
    ASSERT_EQ(min(minVal, i), minVal);

    ASSERT_EQ(max(maxVal, i), maxVal);
    ASSERT_EQ(max(i, maxVal), maxVal);
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
            const auto resInt = lmpd_constrain<int>(15, maxA, minA);
          },
          ".*invalid parameters.*");

  ASSERT_DEATH(
          {
            const int minA = 1000;
            const int maxA = 1000;
            const auto resInt = lmpd_constrain<int>(15, maxA, minA);
          },
          ".*invalid parameters.*");
}

TEST(test_constrain, same_type)
{
  const int minA = -1000;
  const int maxA = 1000;
  int a;
  for (a = minA; a < maxA; a++)
  {
    const auto resInt = lmpd_constrain<int>(a, minA, maxA);
    ASSERT_EQ(resInt, a);
    ASSERT_EQ(typeid(resInt), typeid(a));
  }
}

TEST(test_constrain, negative_size)
{
  const int minA = -1000;
  const int maxA = 1000;

  for (int a = 0; a < maxA; a++)
  {
    const auto res = lmpd_constrain<int>(a, minA, maxA);
    ASSERT_EQ(res, a);
    ASSERT_EQ(typeid(res), typeid(a));
  }
}

TEST(test_constrain, invalidvals)
{
  // not a number
  const auto resNan = lmpd_constrain<float>(NAN, -100.0f, 100.0f);
  ASSERT_EQ(resNan, -100.0f);

  // infinity
  const auto resInf = lmpd_constrain<float>(Inf, -100.0f, 100.0f);
  ASSERT_EQ(resInf, 100.0f);
  const auto resMInf = lmpd_constrain<float>(-Inf, -100.0f, 100.0f);
  ASSERT_EQ(resMInf, -100.0f);

  const auto resMinInf = lmpd_constrain<float>(-1000000.0f, -Inf, 100.0f);
  ASSERT_EQ(resMinInf, -1000000.0f);
  const auto resMaxInf = lmpd_constrain<float>(1000000.0f, 100.0f, Inf);
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
    const auto resA = lmpd_map<int>(a, minA, maxA, minA, maxA);
    ASSERT_EQ(resA, a);

    const auto resB = lmpd_map<int>(a, minA, maxA, maxA, minA);
    ASSERT_EQ(resB, -a);
  }

  const float minB = -1000.0f;
  const float maxB = 1000.0f;
  for (float a = minB; a < maxB; a += 0.5f)
  {
    const auto resA = lmpd_map<float>(a, minB, maxB, minB, maxB);
    ASSERT_EQ(resA, a);

    const auto resB = lmpd_map<float>(a, minB, maxB, maxB, minB);
    ASSERT_EQ(resB, -a);
  }
}

TEST(test_map, positive_to_negative)
{
  size_t i = 0;
  size_t minA = 0;
  size_t minB = 100;
  const auto resA = lmpd_map<float>(i, minA, minB, -1.0f, 1.0f);
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

  ASSERT_DEATH({ lmpd_map<uint8_t>(0.0f, 0.0f, 1.0f, -1.0f, 1.0f); }, ".*out_min invalid.*");
  ASSERT_DEATH({ lmpd_map<uint8_t>(0.0f, 0.0f, 1.0f, 257, 1.0f); }, ".*out_min invalid.*");

  ASSERT_DEATH({ lmpd_map<uint8_t>(0.0f, 0.0f, 1.0f, 0.0f, -1); }, ".*out_max invalid.*");
  ASSERT_DEATH({ lmpd_map<uint8_t>(0.0f, 0.0f, 1.0f, 0.0f, 257); }, ".*out_max invalid.*");
}

TEST(test_map, type_change)
{
  int minA = -200;
  int maxA = 200;
  for (int i = 0; i < maxA; i++)
  {
    const auto resA = lmpd_map<int>(0.0f, 0.0f, 1.0f, i, maxA);
    ASSERT_EQ(resA, i);
    ASSERT_EQ(typeid(resA), typeid(i));
    //
    const auto resB = lmpd_map<int>(1.0f, 0.0f, 1.0f, minA, i);
    ASSERT_EQ(resB, i);
    ASSERT_EQ(typeid(resB), typeid(i));
  }
}

TEST(test_map, uint_reversed_borns)
{
  // check the case where both units are uint, and borns are inverted
  const auto resA = lmpd_map<uint32_t>(1, 0, 255, 142, 33);
  ASSERT_EQ(resA, 141);
  const auto resB = lmpd_map<uint32_t>(60, 0, 255, 142, 33);
  ASSERT_EQ(resB, 116);
}

TEST(test_map, uint8_overflow)
{
  // overflow clamps the result
  const auto resA = lmpd_map<uint8_t>(2000, 0, 1024, 0, 255);
  ASSERT_EQ(resA, 255);

  const auto resB = lmpd_map<uint8_t>(-150, 0, 1024, 0, 255);
  ASSERT_EQ(resB, 0);
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

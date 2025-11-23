#include <cmath>
#include <cstdint>
#include <gtest/gtest.h>
#include <limits>
#include "src/system/utils/utils.h"

static constexpr float Inf = std::numeric_limits<float>::infinity();

/**
 * Test min
 */

TEST(test_min_max, invalid_values)
{
  ASSERT_DEATH({ min<float>(NAN, 0.0f); }, ".*invalid param a.*");
  ASSERT_DEATH({ max<float>(NAN, 0.0f); }, ".*invalid param a.*");

  ASSERT_DEATH({ min<float>(0.0f, NAN); }, ".*invalid param b.*");
  ASSERT_DEATH({ max<float>(0.0f, NAN); }, ".*invalid param b.*");

  ASSERT_EQ(min<float>(Inf, 0.0f), 0.0f);
  ASSERT_EQ(min<float>(-Inf, 0.0f), -Inf);

  ASSERT_EQ(max<float>(Inf, 0.0f), Inf);
  ASSERT_EQ(max<float>(-Inf, 0.0f), 0.0);
}

TEST(test_min_max, normal_use)
{
  int minVal = -100;
  int maxVal = -100;
  for (int i = minVal; i < maxVal; i++)
  {
    ASSERT_EQ(min<int>(i, minVal), minVal);
    ASSERT_EQ(min<int>(minVal, i), minVal);

    ASSERT_EQ(max<int>(maxVal, i), maxVal);
    ASSERT_EQ(max<int>(i, maxVal), maxVal);
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

/**
 * tets colors
 */

TEST(test_colors, color_union)
{
  // check that the union color has the correct order

  COLOR c;
  c.blue = 0x05;
  c.green = 0x03;
  c.red = 0x01;
  c.white = 0x07;

  ASSERT_EQ(c.color, 0x07010305);
}

TEST(test_colors, color_gradient)
{
  //
  ASSERT_EQ(utils::get_gradient(0x00000000, 0x000000FF, 0.0f), 0x00000000);
  ASSERT_EQ(utils::get_gradient(0x00000000, 0x000000FF, 0.5f), 0x0000007F);
  ASSERT_EQ(utils::get_gradient(0x00000000, 0x000000FF, 1.0f), 0x000000FF);
  //
  ASSERT_EQ(utils::get_gradient(0x00000000, 0x0000FF00, 0.0f), 0x00000000);
  ASSERT_EQ(utils::get_gradient(0x00000000, 0x0000FF00, 0.5f), 0x00007F00);
  ASSERT_EQ(utils::get_gradient(0x00000000, 0x0000FF00, 1.0f), 0x0000FF00);
  //
  ASSERT_EQ(utils::get_gradient(0x00000000, 0x00FF0000, 0.0f), 0x00000000);
  ASSERT_EQ(utils::get_gradient(0x00000000, 0x00FF0000, 0.5f), 0x007F0000);
  ASSERT_EQ(utils::get_gradient(0x00000000, 0x00FF0000, 1.0f), 0x00FF0000);
  // no whuite level gradient
  ASSERT_EQ(utils::get_gradient(0x00000000, 0xFF000000, 0.0f), 0x00);
  ASSERT_EQ(utils::get_gradient(0x00000000, 0xFF000000, 0.5f), 0x00);
  ASSERT_EQ(utils::get_gradient(0x00000000, 0xFF000000, 1.0f), 0x00);

  // reverse scale gradient
  ASSERT_EQ(utils::get_gradient(0xFFFFFF, 0x000000, 0.0f), 0xFFFFFF);
  ASSERT_EQ(utils::get_gradient(0xFFFFFF, 0x000000, 0.5f), 0x7F7F7F);
  ASSERT_EQ(utils::get_gradient(0xFFFFFF, 0x000000, 1.0f), 0x000000);
  // reverse scale gradient
  ASSERT_EQ(utils::get_gradient(0x222222, 0x888888, 0.0f), 0x222222);
  ASSERT_EQ(utils::get_gradient(0x222222, 0x888888, 0.5f), 0x555555);
  ASSERT_EQ(utils::get_gradient(0x222222, 0x888888, 1.0f), 0x888888);
}

TEST(test_colors, color_blend16)
{
  COLOR c1, c2;
  c1.color = 0x000000;
  c2.color = 0xFFFFFF;
  uint16_t blend = 0;
  ASSERT_EQ(utils::color_blend(c1, c2, blend, true).color, 0x000000);
  blend = UINT16_MAX / 2;
  ASSERT_EQ(utils::color_blend(c1, c2, blend, true).color, 0x7F7F7F);
  blend = UINT16_MAX;
  ASSERT_EQ(utils::color_blend(c1, c2, blend, true).color, 0xFFFFFF);
}

TEST(test_colors, color_blend8)
{
  COLOR c1, c2;
  c1.color = 0x00000000;
  c2.color = 0xFFFFFFFF;
  uint8_t blend = 0;
  ASSERT_EQ(utils::color_blend(c1, c2, blend, false).color, 0x00000000);
  blend = UINT8_MAX / 2;
  ASSERT_EQ(utils::color_blend(c1, c2, blend, false).color, 0x7E7E7E7E);
  blend = UINT8_MAX;
  ASSERT_EQ(utils::color_blend(c1, c2, blend, false).color, 0xFFFFFFFF);
}

TEST(test_colors, color_fade)
{
  COLOR c1;
  c1.color = 0xFFFFFFFF;
  uint8_t amount = 0;
  ASSERT_EQ(utils::color_fade(c1, amount, false).color, 0x00000000);
  amount = UINT8_MAX / 2;
  ASSERT_EQ(utils::color_fade(c1, amount, false).color, 0x7F7F7F7F);
  amount = UINT8_MAX;
  ASSERT_EQ(utils::color_fade(c1, amount, false).color, c1.color);

  c1.color = 0x88888888;
  amount = 0;
  ASSERT_EQ(utils::color_fade(c1, amount, false).color, 0x00000000);
  amount = UINT8_MAX / 2;
  ASSERT_EQ(utils::color_fade(c1, amount, false).color, 0x44444444);
  amount = UINT8_MAX;
  ASSERT_EQ(utils::color_fade(c1, amount, false).color, c1.color);
}

TEST(test_colors, color_add)
{
  COLOR c1, c2;
  c1.color = 0x00000000;
  c2.color = 0x00000000;
  ASSERT_EQ(utils::color_add(c1, c2).color, 0x00000000);
  c2.color = 0x44444444;
  ASSERT_EQ(utils::color_add(c1, c2).color, 0x44444444);
  c2.color = 0xFFFFFFFF;
  ASSERT_EQ(utils::color_add(c1, c2).color, 0xFFFFFFFF);

  //
  c1.color = 0x00000000;
  c2.color = 0x00000000;
  ASSERT_EQ(utils::color_add(c1, c2).color, 0x00000000);
  c1.color = 0x44444444;
  ASSERT_EQ(utils::color_add(c1, c2).color, 0x44444444);
  c1.color = 0xFFFFFFFF;
  ASSERT_EQ(utils::color_add(c1, c2).color, 0xFFFFFFFF);

  //
  c1.color = 0x44444444;
  c2.color = 0x44444444;
  ASSERT_EQ(utils::color_add(c1, c2).color, 0x88888888);
  c1.color = 0x88888888;
  ASSERT_EQ(utils::color_add(c1, c2).color, 0xCCCCCCCC);
  c1.color = 0xFFFFFFFF;
  ASSERT_EQ(utils::color_add(c1, c2).color, 0xFFFFFFFF);
}

TEST(test_colors, rgb_to_hue)
{
  ASSERT_EQ(utils::hue_to_rgb_sinus(0), 0xFF0000);
  ASSERT_EQ(utils::hue_to_rgb_sinus(200), 0x003FBF);
  ASSERT_EQ(utils::hue_to_rgb_sinus(300), 0x7F007F);
  // overflow
  ASSERT_EQ(utils::hue_to_rgb_sinus(360), 0xFF0000);
  ASSERT_EQ(utils::hue_to_rgb_sinus(360 * 2), 0xFF0000);
  ASSERT_EQ(utils::hue_to_rgb_sinus(560), 0x003FBF);
}

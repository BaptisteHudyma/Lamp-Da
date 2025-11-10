#include <cmath>
#include <cstdint>
#include <gtest/gtest.h>
#include <limits>
#include "src/system/utils/vector_math.h"

static constexpr float Inf = std::numeric_limits<float>::infinity();

TEST(test_vector2d, init)
{
  vec2d vec1;
  ASSERT_EQ(vec1.x, 0.0f);
  ASSERT_EQ(vec1.y, 0.0f);

  vec2d vec2(50.05f, -52.0f);
  ASSERT_EQ(vec2.x, 50.05f);
  ASSERT_EQ(vec2.y, -52.0f);

  // check =
  vec1 = vec2;
  ASSERT_EQ(vec1.x, 50.05f);
  ASSERT_EQ(vec1.y, -52.0f);

  vec2d vec3(vec1);
  ASSERT_EQ(vec3.x, 50.05f);
  ASSERT_EQ(vec3.y, -52.0f);

  // test auto equal
  vec2 = vec2;
  ASSERT_EQ(vec3.x, 50.05f);
  ASSERT_EQ(vec3.y, -52.0f);
}

TEST(test_vector2d, add)
{
  vec2d vec1(-1.0f, -2.0f);
  vec2d vec2(1.0f, 2.0f);

  vec2d res1 = vec1.add(vec2);
  ASSERT_EQ(vec1.x, -1.0f);
  ASSERT_EQ(vec1.y, -2.0f);
  ASSERT_EQ(vec2.x, 1.0f);
  ASSERT_EQ(vec2.y, 2.0f);
  ASSERT_EQ(res1.x, 0.0f);
  ASSERT_EQ(res1.y, 0.0f);

  vec2d res2 = vec1.add(vec1);
  ASSERT_EQ(vec1.x, -1.0f);
  ASSERT_EQ(vec1.y, -2.0f);
  ASSERT_EQ(res2.x, -2.0f);
  ASSERT_EQ(res2.y, -4.0f);

  vec2d res3 = vec2.add(vec2);
  ASSERT_EQ(vec2.x, 1.0f);
  ASSERT_EQ(vec2.y, 2.0f);
  ASSERT_EQ(res3.x, 2.0f);
  ASSERT_EQ(res3.y, 4.0f);

  // add constant
  vec2d res4 = vec1.add(1.0);
  ASSERT_EQ(vec1.x, -1.0f);
  ASSERT_EQ(vec1.y, -2.0f);
  ASSERT_EQ(res4.x, 0.0f);
  ASSERT_EQ(res4.y, -1.0f);

  vec2d res5 = vec2.add(-1.0);
  ASSERT_EQ(vec2.x, 1.0f);
  ASSERT_EQ(vec2.y, 2.0f);
  ASSERT_EQ(res5.x, 0.0f);
  ASSERT_EQ(res5.y, 1.0f);
}

TEST(test_vector2d, dot)
{
  vec2d vec1(-1.0f, -2.0f);
  vec2d vec2(1.0f, 2.0f);

  float res1 = vec1.dot(vec1);
  ASSERT_EQ(vec1.x, -1.0f);
  ASSERT_EQ(vec1.y, -2.0f);
  ASSERT_EQ(res1, 5.0f);

  float res2 = vec2.dot(vec2);
  ASSERT_EQ(vec2.x, 1.0f);
  ASSERT_EQ(vec2.y, 2.0f);
  ASSERT_EQ(res2, 5.0f);

  float res3 = vec1.dot(vec2);
  ASSERT_EQ(vec1.x, -1.0f);
  ASSERT_EQ(vec1.y, -2.0f);
  ASSERT_EQ(vec2.x, 1.0f);
  ASSERT_EQ(vec2.y, 2.0f);
  ASSERT_EQ(res3, -5.0f);

  float res4 = vec2.dot(vec1);
  ASSERT_EQ(vec1.x, -1.0f);
  ASSERT_EQ(vec1.y, -2.0f);
  ASSERT_EQ(vec2.x, 1.0f);
  ASSERT_EQ(vec2.y, 2.0f);
  ASSERT_EQ(res4, -5.0f);
}

TEST(test_vector2d, multiply)
{
  vec2d vec1(-1.0f, -2.0f);
  vec2d vec2(1.0f, 2.0f);

  vec2d res1 = vec1.multiply(vec1);
  ASSERT_EQ(vec1.x, -1.0f);
  ASSERT_EQ(vec1.y, -2.0f);
  ASSERT_EQ(res1.x, 1.0f);
  ASSERT_EQ(res1.y, 4.0f);

  vec2d res2 = vec2.multiply(vec2);
  ASSERT_EQ(vec2.x, 1.0f);
  ASSERT_EQ(vec2.y, 2.0f);
  ASSERT_EQ(res2.x, 1.0f);
  ASSERT_EQ(res2.y, 4.0f);

  vec2d res3 = vec1.multiply(vec2);
  ASSERT_EQ(vec1.x, -1.0f);
  ASSERT_EQ(vec1.y, -2.0f);
  ASSERT_EQ(vec2.x, 1.0f);
  ASSERT_EQ(vec2.y, 2.0f);
  ASSERT_EQ(res3.x, -1.0f);
  ASSERT_EQ(res3.y, -4.0f);

  vec2d res4 = vec2.multiply(vec1);
  ASSERT_EQ(vec1.x, -1.0f);
  ASSERT_EQ(vec1.y, -2.0f);
  ASSERT_EQ(vec2.x, 1.0f);
  ASSERT_EQ(vec2.y, 2.0f);
  ASSERT_EQ(res4.x, -1.0f);
  ASSERT_EQ(res4.y, -4.0f);

  // constant
  vec2d res5 = vec1.multiply(1.0f);
  ASSERT_EQ(vec1.x, -1.0f);
  ASSERT_EQ(vec1.y, -2.0f);
  ASSERT_EQ(res5.x, -1.0f);
  ASSERT_EQ(res5.y, -2.0f);

  vec2d res6 = vec1.multiply(-1.0f);
  ASSERT_EQ(vec1.x, -1.0f);
  ASSERT_EQ(vec1.y, -2.0f);
  ASSERT_EQ(res6.x, 1.0f);
  ASSERT_EQ(res6.y, 2.0f);
}

// Vector 3

TEST(test_vector3d, init)
{
  vec3d vec1;
  ASSERT_EQ(vec1.x, 0.0f);
  ASSERT_EQ(vec1.y, 0.0f);
  ASSERT_EQ(vec1.z, 0.0f);

  vec3d vec2(50.05f, -52.0f, 10.0f);
  ASSERT_EQ(vec2.x, 50.05f);
  ASSERT_EQ(vec2.y, -52.0f);
  ASSERT_EQ(vec2.z, 10.0f);

  // check =
  vec1 = vec2;
  ASSERT_EQ(vec1.x, 50.05f);
  ASSERT_EQ(vec1.y, -52.0f);
  ASSERT_EQ(vec1.z, 10.0f);

  vec3d vec3(vec1);
  ASSERT_EQ(vec3.x, 50.05f);
  ASSERT_EQ(vec3.y, -52.0f);
  ASSERT_EQ(vec3.z, 10.0f);

  // test auto equal
  vec2 = vec2;
  ASSERT_EQ(vec3.x, 50.05f);
  ASSERT_EQ(vec3.y, -52.0f);
  ASSERT_EQ(vec3.z, 10.0f);
}

TEST(test_vector3d, add)
{
  vec3d vec1(-1.0f, -2.0f, -3.0f);
  vec3d vec2(1.0f, 2.0f, 3.0f);

  vec3d res1 = vec1.add(vec2);
  ASSERT_EQ(vec1.x, -1.0f);
  ASSERT_EQ(vec1.y, -2.0f);
  ASSERT_EQ(vec1.z, -3.0f);
  ASSERT_EQ(vec2.x, 1.0f);
  ASSERT_EQ(vec2.y, 2.0f);
  ASSERT_EQ(vec2.z, 3.0f);
  ASSERT_EQ(res1.x, 0.0f);
  ASSERT_EQ(res1.y, 0.0f);
  ASSERT_EQ(res1.z, 0.0f);

  vec3d res2 = vec1.add(vec1);
  ASSERT_EQ(vec1.x, -1.0f);
  ASSERT_EQ(vec1.y, -2.0f);
  ASSERT_EQ(vec1.z, -3.0f);
  ASSERT_EQ(res2.x, -2.0f);
  ASSERT_EQ(res2.y, -4.0f);
  ASSERT_EQ(res2.z, -6.0f);

  vec3d res3 = vec2.add(vec2);
  ASSERT_EQ(vec2.x, 1.0f);
  ASSERT_EQ(vec2.y, 2.0f);
  ASSERT_EQ(vec2.z, 3.0f);
  ASSERT_EQ(res3.x, 2.0f);
  ASSERT_EQ(res3.y, 4.0f);
  ASSERT_EQ(res3.z, 6.0f);

  // add constant
  vec3d res4 = vec1.add(1.0);
  ASSERT_EQ(vec1.x, -1.0f);
  ASSERT_EQ(vec1.y, -2.0f);
  ASSERT_EQ(vec1.z, -3.0f);
  ASSERT_EQ(res4.x, 0.0f);
  ASSERT_EQ(res4.y, -1.0f);
  ASSERT_EQ(res4.z, -2.0f);

  vec3d res5 = vec2.add(-1.0);
  ASSERT_EQ(vec2.x, 1.0f);
  ASSERT_EQ(vec2.y, 2.0f);
  ASSERT_EQ(vec2.z, 3.0f);
  ASSERT_EQ(res5.x, 0.0f);
  ASSERT_EQ(res5.y, 1.0f);
  ASSERT_EQ(res5.z, 2.0f);
}

TEST(test_vector3d, dot)
{
  vec3d vec1(-1.0f, -2.0f, -3.0f);
  vec3d vec2(1.0f, 2.0f, 3.0f);

  float res1 = vec1.dot(vec1);
  ASSERT_EQ(vec1.x, -1.0f);
  ASSERT_EQ(vec1.y, -2.0f);
  ASSERT_EQ(vec1.z, -3.0f);
  ASSERT_EQ(res1, 14.0f);

  float res2 = vec2.dot(vec2);
  ASSERT_EQ(vec2.x, 1.0f);
  ASSERT_EQ(vec2.y, 2.0f);
  ASSERT_EQ(vec2.z, 3.0f);
  ASSERT_EQ(res2, 14.0f);

  float res3 = vec1.dot(vec2);
  ASSERT_EQ(vec1.x, -1.0f);
  ASSERT_EQ(vec1.y, -2.0f);
  ASSERT_EQ(vec1.z, -3.0f);
  ASSERT_EQ(vec2.x, 1.0f);
  ASSERT_EQ(vec2.y, 2.0f);
  ASSERT_EQ(vec2.z, 3.0f);
  ASSERT_EQ(res3, -14.0f);

  float res4 = vec2.dot(vec1);
  ASSERT_EQ(vec1.x, -1.0f);
  ASSERT_EQ(vec1.y, -2.0f);
  ASSERT_EQ(vec1.z, -3.0f);
  ASSERT_EQ(vec2.x, 1.0f);
  ASSERT_EQ(vec2.y, 2.0f);
  ASSERT_EQ(vec2.z, 3.0f);
  ASSERT_EQ(res4, -14.0f);
}

TEST(test_vector3d, multiply)
{
  vec3d vec1(-1.0f, -2.0f, -3.0f);
  vec3d vec2(1.0f, 2.0f, 3.0f);

  vec3d res1 = vec1.multiply(vec1);
  ASSERT_EQ(vec1.x, -1.0f);
  ASSERT_EQ(vec1.y, -2.0f);
  ASSERT_EQ(vec1.z, -3.0f);
  ASSERT_EQ(res1.x, 1.0f);
  ASSERT_EQ(res1.y, 4.0f);
  ASSERT_EQ(res1.z, 9.0f);

  vec3d res2 = vec2.multiply(vec2);
  ASSERT_EQ(vec2.x, 1.0f);
  ASSERT_EQ(vec2.y, 2.0f);
  ASSERT_EQ(vec2.z, 3.0f);
  ASSERT_EQ(res2.x, 1.0f);
  ASSERT_EQ(res2.y, 4.0f);
  ASSERT_EQ(res2.z, 9.0f);

  vec3d res3 = vec1.multiply(vec2);
  ASSERT_EQ(vec1.x, -1.0f);
  ASSERT_EQ(vec1.y, -2.0f);
  ASSERT_EQ(vec1.z, -3.0f);
  ASSERT_EQ(vec2.x, 1.0f);
  ASSERT_EQ(vec2.y, 2.0f);
  ASSERT_EQ(vec2.z, 3.0f);
  ASSERT_EQ(res3.x, -1.0f);
  ASSERT_EQ(res3.y, -4.0f);
  ASSERT_EQ(res3.z, -9.0f);

  vec3d res4 = vec2.multiply(vec1);
  ASSERT_EQ(vec1.x, -1.0f);
  ASSERT_EQ(vec1.y, -2.0f);
  ASSERT_EQ(vec1.z, -3.0f);
  ASSERT_EQ(vec2.x, 1.0f);
  ASSERT_EQ(vec2.y, 2.0f);
  ASSERT_EQ(vec2.z, 3.0f);
  ASSERT_EQ(res4.x, -1.0f);
  ASSERT_EQ(res4.y, -4.0f);
  ASSERT_EQ(res4.z, -9.0f);

  // constant
  vec3d res5 = vec1.multiply(1.0f);
  ASSERT_EQ(vec1.x, -1.0f);
  ASSERT_EQ(vec1.y, -2.0f);
  ASSERT_EQ(vec1.z, -3.0f);
  ASSERT_EQ(res5.x, -1.0f);
  ASSERT_EQ(res5.y, -2.0f);
  ASSERT_EQ(res5.z, -3.0f);

  vec3d res6 = vec1.multiply(-1.0f);
  ASSERT_EQ(vec1.x, -1.0f);
  ASSERT_EQ(vec1.y, -2.0f);
  ASSERT_EQ(vec1.z, -3.0f);
  ASSERT_EQ(res6.x, 1.0f);
  ASSERT_EQ(res6.y, 2.0f);
  ASSERT_EQ(res6.z, 3.0f);
}

// Vector 4

TEST(test_vector4d, init)
{
  vec4d vec1;
  ASSERT_EQ(vec1.x, 0.0f);
  ASSERT_EQ(vec1.y, 0.0f);
  ASSERT_EQ(vec1.z, 0.0f);
  ASSERT_EQ(vec1.w, 0.0f);

  vec4d vec2(50.05f, -52.0f, 10.0f, 115.0f);
  ASSERT_EQ(vec2.x, 50.05f);
  ASSERT_EQ(vec2.y, -52.0f);
  ASSERT_EQ(vec2.z, 10.0f);
  ASSERT_EQ(vec2.w, 115.0f);

  // check =
  vec1 = vec2;
  ASSERT_EQ(vec1.x, 50.05f);
  ASSERT_EQ(vec1.y, -52.0f);
  ASSERT_EQ(vec1.z, 10.0f);
  ASSERT_EQ(vec1.w, 115.0f);

  vec4d vec3(vec1);
  ASSERT_EQ(vec3.x, 50.05f);
  ASSERT_EQ(vec3.y, -52.0f);
  ASSERT_EQ(vec3.z, 10.0f);
  ASSERT_EQ(vec3.w, 115.0f);

  // test auto equal
  vec2 = vec2;
  ASSERT_EQ(vec3.x, 50.05f);
  ASSERT_EQ(vec3.y, -52.0f);
  ASSERT_EQ(vec3.z, 10.0f);
  ASSERT_EQ(vec3.w, 115.0f);
}

TEST(test_vector4d, add)
{
  vec4d vec1(-1.0f, -2.0f, -3.0f, -4.0f);
  vec4d vec2(1.0f, 2.0f, 3.0f, 4.0f);

  vec4d res1 = vec1.add(vec2);
  ASSERT_EQ(vec1.x, -1.0f);
  ASSERT_EQ(vec1.y, -2.0f);
  ASSERT_EQ(vec1.z, -3.0f);
  ASSERT_EQ(vec1.w, -4.0f);
  ASSERT_EQ(vec2.x, 1.0f);
  ASSERT_EQ(vec2.y, 2.0f);
  ASSERT_EQ(vec2.z, 3.0f);
  ASSERT_EQ(vec2.w, 4.0f);
  ASSERT_EQ(res1.x, 0.0f);
  ASSERT_EQ(res1.y, 0.0f);
  ASSERT_EQ(res1.z, 0.0f);
  ASSERT_EQ(res1.w, 0.0f);

  vec4d res2 = vec1.add(vec1);
  ASSERT_EQ(vec1.x, -1.0f);
  ASSERT_EQ(vec1.y, -2.0f);
  ASSERT_EQ(vec1.z, -3.0f);
  ASSERT_EQ(vec1.w, -4.0f);
  ASSERT_EQ(res2.x, -2.0f);
  ASSERT_EQ(res2.y, -4.0f);
  ASSERT_EQ(res2.z, -6.0f);
  ASSERT_EQ(res2.w, -8.0f);

  vec4d res3 = vec2.add(vec2);
  ASSERT_EQ(vec2.x, 1.0f);
  ASSERT_EQ(vec2.y, 2.0f);
  ASSERT_EQ(vec2.z, 3.0f);
  ASSERT_EQ(vec2.w, 4.0f);
  ASSERT_EQ(res3.x, 2.0f);
  ASSERT_EQ(res3.y, 4.0f);
  ASSERT_EQ(res3.z, 6.0f);
  ASSERT_EQ(res3.w, 8.0f);

  // add constant
  vec4d res4 = vec1.add(1.0);
  ASSERT_EQ(vec1.x, -1.0f);
  ASSERT_EQ(vec1.y, -2.0f);
  ASSERT_EQ(vec1.z, -3.0f);
  ASSERT_EQ(vec1.w, -4.0f);
  ASSERT_EQ(res4.x, 0.0f);
  ASSERT_EQ(res4.y, -1.0f);
  ASSERT_EQ(res4.z, -2.0f);
  ASSERT_EQ(res4.w, -3.0f);

  vec4d res5 = vec2.add(-1.0);
  ASSERT_EQ(vec2.x, 1.0f);
  ASSERT_EQ(vec2.y, 2.0f);
  ASSERT_EQ(vec2.z, 3.0f);
  ASSERT_EQ(vec2.w, 4.0f);
  ASSERT_EQ(res5.x, 0.0f);
  ASSERT_EQ(res5.y, 1.0f);
  ASSERT_EQ(res5.z, 2.0f);
  ASSERT_EQ(res5.w, 3.0f);
}

TEST(test_vector4d, dot)
{
  vec4d vec1(-1.0f, -2.0f, -3.0f, -4.0f);
  vec4d vec2(1.0f, 2.0f, 3.0f, 4.0f);

  float res1 = vec1.dot(vec1);
  ASSERT_EQ(vec1.x, -1.0f);
  ASSERT_EQ(vec1.y, -2.0f);
  ASSERT_EQ(vec1.z, -3.0f);
  ASSERT_EQ(vec1.w, -4.0f);
  ASSERT_EQ(res1, 30.0f);

  float res2 = vec2.dot(vec2);
  ASSERT_EQ(vec2.x, 1.0f);
  ASSERT_EQ(vec2.y, 2.0f);
  ASSERT_EQ(vec2.z, 3.0f);
  ASSERT_EQ(vec2.w, 4.0f);
  ASSERT_EQ(res2, 30.0f);

  float res3 = vec1.dot(vec2);
  ASSERT_EQ(vec1.x, -1.0f);
  ASSERT_EQ(vec1.y, -2.0f);
  ASSERT_EQ(vec1.z, -3.0f);
  ASSERT_EQ(vec1.w, -4.0f);
  ASSERT_EQ(vec2.x, 1.0f);
  ASSERT_EQ(vec2.y, 2.0f);
  ASSERT_EQ(vec2.z, 3.0f);
  ASSERT_EQ(vec2.w, 4.0f);
  ASSERT_EQ(res3, -30.0f);

  float res4 = vec2.dot(vec1);
  ASSERT_EQ(vec1.x, -1.0f);
  ASSERT_EQ(vec1.y, -2.0f);
  ASSERT_EQ(vec1.z, -3.0f);
  ASSERT_EQ(vec1.w, -4.0f);
  ASSERT_EQ(vec2.x, 1.0f);
  ASSERT_EQ(vec2.y, 2.0f);
  ASSERT_EQ(vec2.z, 3.0f);
  ASSERT_EQ(vec2.w, 4.0f);
  ASSERT_EQ(res4, -30.0f);
}

TEST(test_vector4d, multiply)
{
  vec4d vec1(-1.0f, -2.0f, -3.0f, -4.0f);
  vec4d vec2(1.0f, 2.0f, 3.0f, 4.0f);

  vec4d res1 = vec1.multiply(vec1);
  ASSERT_EQ(vec1.x, -1.0f);
  ASSERT_EQ(vec1.y, -2.0f);
  ASSERT_EQ(vec1.z, -3.0f);
  ASSERT_EQ(vec1.w, -4.0f);
  ASSERT_EQ(res1.x, 1.0f);
  ASSERT_EQ(res1.y, 4.0f);
  ASSERT_EQ(res1.z, 9.0f);
  ASSERT_EQ(res1.w, 16.0f);

  vec4d res2 = vec2.multiply(vec2);
  ASSERT_EQ(vec2.x, 1.0f);
  ASSERT_EQ(vec2.y, 2.0f);
  ASSERT_EQ(vec2.z, 3.0f);
  ASSERT_EQ(vec2.w, 4.0f);
  ASSERT_EQ(res2.x, 1.0f);
  ASSERT_EQ(res2.y, 4.0f);
  ASSERT_EQ(res2.z, 9.0f);
  ASSERT_EQ(res2.w, 16.0f);

  vec4d res3 = vec1.multiply(vec2);
  ASSERT_EQ(vec1.x, -1.0f);
  ASSERT_EQ(vec1.y, -2.0f);
  ASSERT_EQ(vec1.z, -3.0f);
  ASSERT_EQ(vec1.w, -4.0f);
  ASSERT_EQ(vec2.x, 1.0f);
  ASSERT_EQ(vec2.y, 2.0f);
  ASSERT_EQ(vec2.z, 3.0f);
  ASSERT_EQ(vec2.w, 4.0f);
  ASSERT_EQ(res3.x, -1.0f);
  ASSERT_EQ(res3.y, -4.0f);
  ASSERT_EQ(res3.z, -9.0f);
  ASSERT_EQ(res3.w, -16.0f);

  vec4d res4 = vec2.multiply(vec1);
  ASSERT_EQ(vec1.x, -1.0f);
  ASSERT_EQ(vec1.y, -2.0f);
  ASSERT_EQ(vec1.z, -3.0f);
  ASSERT_EQ(vec1.w, -4.0f);
  ASSERT_EQ(vec2.x, 1.0f);
  ASSERT_EQ(vec2.y, 2.0f);
  ASSERT_EQ(vec2.z, 3.0f);
  ASSERT_EQ(vec2.w, 4.0f);
  ASSERT_EQ(res4.x, -1.0f);
  ASSERT_EQ(res4.y, -4.0f);
  ASSERT_EQ(res4.z, -9.0f);
  ASSERT_EQ(res4.w, -16.0f);

  // constant
  vec4d res5 = vec1.multiply(1.0f);
  ASSERT_EQ(vec1.x, -1.0f);
  ASSERT_EQ(vec1.y, -2.0f);
  ASSERT_EQ(vec1.z, -3.0f);
  ASSERT_EQ(vec1.w, -4.0f);
  ASSERT_EQ(res5.x, -1.0f);
  ASSERT_EQ(res5.y, -2.0f);
  ASSERT_EQ(res5.z, -3.0f);
  ASSERT_EQ(res5.w, -4.0f);

  vec4d res6 = vec1.multiply(-1.0f);
  ASSERT_EQ(vec1.x, -1.0f);
  ASSERT_EQ(vec1.y, -2.0f);
  ASSERT_EQ(vec1.z, -3.0f);
  ASSERT_EQ(vec1.w, -4.0f);
  ASSERT_EQ(res6.x, 1.0f);
  ASSERT_EQ(res6.y, 2.0f);
  ASSERT_EQ(res6.z, 3.0f);
  ASSERT_EQ(res6.w, 4.0f);
}

// Rotation matrix

TEST(test_rotation_matrix, create_zero_rot)
{
  RotationMatrix mat;
  mat.from_angles_XYZ(0.0f, 0.0f, 0.0f);

  ASSERT_EQ(mat.R11, 1.0f);
  ASSERT_EQ(mat.R12, 0.0f);
  ASSERT_EQ(mat.R13, 0.0f);

  ASSERT_EQ(mat.R21, 0.0f);
  ASSERT_EQ(mat.R22, 1.0f);
  ASSERT_EQ(mat.R23, 0.0f);

  ASSERT_EQ(mat.R31, 0.0f);
  ASSERT_EQ(mat.R32, 0.0f);
  ASSERT_EQ(mat.R33, 1.0f);

  // check accessors
  ASSERT_EQ(mat.col1().x, 1.0f);
  ASSERT_EQ(mat.col1().y, 0.0f);
  ASSERT_EQ(mat.col1().z, 0.0f);
  ASSERT_EQ(mat.row1().x, 1.0f);
  ASSERT_EQ(mat.row1().y, 0.0f);
  ASSERT_EQ(mat.row1().z, 0.0f);

  ASSERT_EQ(mat.col2().x, 0.0f);
  ASSERT_EQ(mat.col2().y, 1.0f);
  ASSERT_EQ(mat.col2().z, 0.0f);
  ASSERT_EQ(mat.row2().x, 0.0f);
  ASSERT_EQ(mat.row2().y, 1.0f);
  ASSERT_EQ(mat.row2().z, 0.0f);

  ASSERT_EQ(mat.col3().x, 0.0f);
  ASSERT_EQ(mat.col3().y, 0.0f);
  ASSERT_EQ(mat.col3().z, 1.0f);
  ASSERT_EQ(mat.row3().x, 0.0f);
  ASSERT_EQ(mat.row3().y, 0.0f);
  ASSERT_EQ(mat.row3().z, 1.0f);

  // compose
  RotationMatrix res1 = mat.compose(mat);
  ASSERT_EQ(res1.R11, 1.0f);
  ASSERT_EQ(res1.R12, 0.0f);
  ASSERT_EQ(res1.R13, 0.0f);

  ASSERT_EQ(res1.R21, 0.0f);
  ASSERT_EQ(res1.R22, 1.0f);
  ASSERT_EQ(res1.R23, 0.0f);

  ASSERT_EQ(res1.R31, 0.0f);
  ASSERT_EQ(res1.R32, 0.0f);
  ASSERT_EQ(res1.R33, 1.0f);

  // rotate vector
  vec3d vecA(2.0f, 5.0f, 11.0f);
  vec3d res2 = mat.transform(vecA);
  ASSERT_EQ(res2.x, vecA.x);
  ASSERT_EQ(res2.y, vecA.y);
  ASSERT_EQ(res2.z, vecA.z);

  vecA = vec3d(-2.0f, -5.0f, -11.0f);
  res2 = mat.transform(vecA);
  ASSERT_EQ(res2.x, vecA.x);
  ASSERT_EQ(res2.y, vecA.y);
  ASSERT_EQ(res2.z, vecA.z);
}

TEST(test_rotation_matrix, create_360_rot)
{
  RotationMatrix mat;
  mat.from_angles_XYZ(2.0f * M_PIf, 2.0f * M_PIf, 2.0f * M_PIf);

  ASSERT_NEAR(mat.R11, 1.0f, 0.00001);
  ASSERT_NEAR(mat.R12, 0.0f, 0.00001);
  ASSERT_NEAR(mat.R13, 0.0f, 0.00001);

  ASSERT_NEAR(mat.R21, 0.0f, 0.00001);
  ASSERT_NEAR(mat.R22, 1.0f, 0.00001);
  ASSERT_NEAR(mat.R23, 0.0f, 0.00001);

  ASSERT_NEAR(mat.R31, 0.0f, 0.00001);
  ASSERT_NEAR(mat.R32, 0.0f, 0.00001);
  ASSERT_NEAR(mat.R33, 1.0f, 0.00001);

  // check accessors
  ASSERT_NEAR(mat.col1().x, 1.0f, 0.00001);
  ASSERT_NEAR(mat.col1().y, 0.0f, 0.00001);
  ASSERT_NEAR(mat.col1().z, 0.0f, 0.00001);
  ASSERT_NEAR(mat.row1().x, 1.0f, 0.00001);
  ASSERT_NEAR(mat.row1().y, 0.0f, 0.00001);
  ASSERT_NEAR(mat.row1().z, 0.0f, 0.00001);

  ASSERT_NEAR(mat.col2().x, 0.0f, 0.00001);
  ASSERT_NEAR(mat.col2().y, 1.0f, 0.00001);
  ASSERT_NEAR(mat.col2().z, 0.0f, 0.00001);
  ASSERT_NEAR(mat.row2().x, 0.0f, 0.00001);
  ASSERT_NEAR(mat.row2().y, 1.0f, 0.00001);
  ASSERT_NEAR(mat.row2().z, 0.0f, 0.00001);

  ASSERT_NEAR(mat.col3().x, 0.0f, 0.00001);
  ASSERT_NEAR(mat.col3().y, 0.0f, 0.00001);
  ASSERT_NEAR(mat.col3().z, 1.0f, 0.00001);
  ASSERT_NEAR(mat.row3().x, 0.0f, 0.00001);
  ASSERT_NEAR(mat.row3().y, 0.0f, 0.00001);
  ASSERT_NEAR(mat.row3().z, 1.0f, 0.00001);

  // compose
  RotationMatrix res1 = mat.compose(mat);
  ASSERT_NEAR(res1.R11, 1.0f, 0.00001);
  ASSERT_NEAR(res1.R12, 0.0f, 0.00001);
  ASSERT_NEAR(res1.R13, 0.0f, 0.00001);

  ASSERT_NEAR(res1.R21, 0.0f, 0.00001);
  ASSERT_NEAR(res1.R22, 1.0f, 0.00001);
  ASSERT_NEAR(res1.R23, 0.0f, 0.00001);

  ASSERT_NEAR(res1.R31, 0.0f, 0.00001);
  ASSERT_NEAR(res1.R32, 0.0f, 0.00001);
  ASSERT_NEAR(res1.R33, 1.0f, 0.00001);

  // rotate vector
  vec3d vecA(2.0f, 5.0f, 11.0f);
  vec3d res2 = mat.transform(vecA);
  ASSERT_NEAR(res2.x, vecA.x, 0.00001);
  ASSERT_NEAR(res2.y, vecA.y, 0.00001);
  ASSERT_NEAR(res2.z, vecA.z, 0.00001);

  vecA = vec3d(-2.0f, -5.0f, -11.0f);
  res2 = mat.transform(vecA);
  ASSERT_NEAR(res2.x, vecA.x, 0.00001);
  ASSERT_NEAR(res2.y, vecA.y, 0.00001);
  ASSERT_NEAR(res2.z, vecA.z, 0.00001);
}

TEST(test_rotation_matrix, create_180_rot)
{
  RotationMatrix mat;
  mat.from_angles_XYZ(M_PIf, M_PIf, M_PIf);

  ASSERT_NEAR(mat.R11, 1.0f, 0.00001);
  ASSERT_NEAR(mat.R12, 0.0f, 0.00001);
  ASSERT_NEAR(mat.R13, 0.0f, 0.00001);

  ASSERT_NEAR(mat.R21, 0.0f, 0.00001);
  ASSERT_NEAR(mat.R22, 1.0f, 0.00001);
  ASSERT_NEAR(mat.R23, 0.0f, 0.00001);

  ASSERT_NEAR(mat.R31, 0.0f, 0.00001);
  ASSERT_NEAR(mat.R32, 0.0f, 0.00001);
  ASSERT_NEAR(mat.R33, 1.0f, 0.00001);

  // check accessors
  ASSERT_NEAR(mat.col1().x, 1.0f, 0.00001);
  ASSERT_NEAR(mat.col1().y, 0.0f, 0.00001);
  ASSERT_NEAR(mat.col1().z, 0.0f, 0.00001);
  ASSERT_NEAR(mat.row1().x, 1.0f, 0.00001);
  ASSERT_NEAR(mat.row1().y, 0.0f, 0.00001);
  ASSERT_NEAR(mat.row1().z, 0.0f, 0.00001);

  ASSERT_NEAR(mat.col2().x, 0.0f, 0.00001);
  ASSERT_NEAR(mat.col2().y, 1.0f, 0.00001);
  ASSERT_NEAR(mat.col2().z, 0.0f, 0.00001);
  ASSERT_NEAR(mat.row2().x, 0.0f, 0.00001);
  ASSERT_NEAR(mat.row2().y, 1.0f, 0.00001);
  ASSERT_NEAR(mat.row2().z, 0.0f, 0.00001);

  ASSERT_NEAR(mat.col3().x, 0.0f, 0.00001);
  ASSERT_NEAR(mat.col3().y, 0.0f, 0.00001);
  ASSERT_NEAR(mat.col3().z, 1.0f, 0.00001);
  ASSERT_NEAR(mat.row3().x, 0.0f, 0.00001);
  ASSERT_NEAR(mat.row3().y, 0.0f, 0.00001);
  ASSERT_NEAR(mat.row3().z, 1.0f, 0.00001);

  // compose
  RotationMatrix res1 = mat.compose(mat);
  ASSERT_NEAR(res1.R11, 1.0f, 0.00001);
  ASSERT_NEAR(res1.R12, 0.0f, 0.00001);
  ASSERT_NEAR(res1.R13, 0.0f, 0.00001);

  ASSERT_NEAR(res1.R21, 0.0f, 0.00001);
  ASSERT_NEAR(res1.R22, 1.0f, 0.00001);
  ASSERT_NEAR(res1.R23, 0.0f, 0.00001);

  ASSERT_NEAR(res1.R31, 0.0f, 0.00001);
  ASSERT_NEAR(res1.R32, 0.0f, 0.00001);
  ASSERT_NEAR(res1.R33, 1.0f, 0.00001);

  // rotate vector
  vec3d vecA(2.0f, 5.0f, 11.0f);
  vec3d res2 = mat.transform(vecA);
  ASSERT_NEAR(res2.x, vecA.x, 0.00001);
  ASSERT_NEAR(res2.y, vecA.y, 0.00001);
  ASSERT_NEAR(res2.z, vecA.z, 0.00001);

  vecA = vec3d(-2.0f, -5.0f, -11.0f);
  res2 = mat.transform(vecA);
  ASSERT_NEAR(res2.x, vecA.x, 0.00001);
  ASSERT_NEAR(res2.y, vecA.y, 0.00001);
  ASSERT_NEAR(res2.z, vecA.z, 0.00001);
}

TEST(test_rotation_matrix, create_90_rot)
{
  RotationMatrix mat;
  mat.from_angles_XYZ(M_PIf / 2.0f, M_PIf / 2.0f, M_PIf / 2.0f);

  ASSERT_NEAR(mat.R11, 0.0f, 0.00001);
  ASSERT_NEAR(mat.R12, 0.0f, 0.00001);
  ASSERT_NEAR(mat.R13, 1.0f, 0.00001);

  ASSERT_NEAR(mat.R21, 0.0f, 0.00001);
  ASSERT_NEAR(mat.R22, 1.0f, 0.00001);
  ASSERT_NEAR(mat.R23, 0.0f, 0.00001);

  ASSERT_NEAR(mat.R31, -1.0f, 0.00001);
  ASSERT_NEAR(mat.R32, 0.0f, 0.00001);
  ASSERT_NEAR(mat.R33, 0.0f, 0.00001);

  // check accessors
  ASSERT_NEAR(mat.col1().x, 0.0f, 0.00001);
  ASSERT_NEAR(mat.col1().y, 0.0f, 0.00001);
  ASSERT_NEAR(mat.col1().z, -1.0f, 0.00001);
  ASSERT_NEAR(mat.row1().x, 0.0f, 0.00001);
  ASSERT_NEAR(mat.row1().y, 0.0f, 0.00001);
  ASSERT_NEAR(mat.row1().z, 1.0f, 0.00001);

  ASSERT_NEAR(mat.col2().x, 0.0f, 0.00001);
  ASSERT_NEAR(mat.col2().y, 1.0f, 0.00001);
  ASSERT_NEAR(mat.col2().z, 0.0f, 0.00001);
  ASSERT_NEAR(mat.row2().x, 0.0f, 0.00001);
  ASSERT_NEAR(mat.row2().y, 1.0f, 0.00001);
  ASSERT_NEAR(mat.row2().z, 0.0f, 0.00001);

  ASSERT_NEAR(mat.col3().x, 1.0f, 0.00001);
  ASSERT_NEAR(mat.col3().y, 0.0f, 0.00001);
  ASSERT_NEAR(mat.col3().z, 0.0f, 0.00001);
  ASSERT_NEAR(mat.row3().x, -1.0f, 0.00001);
  ASSERT_NEAR(mat.row3().y, 0.0f, 0.00001);
  ASSERT_NEAR(mat.row3().z, 0.0f, 0.00001);

  // compose
  RotationMatrix res1 = mat.compose(mat);
  ASSERT_NEAR(res1.R11, -1.0f, 0.00001);
  ASSERT_NEAR(res1.R12, 0.0f, 0.00001);
  ASSERT_NEAR(res1.R13, 0.0f, 0.00001);

  ASSERT_NEAR(res1.R21, 0.0f, 0.00001);
  ASSERT_NEAR(res1.R22, 1.0f, 0.00001);
  ASSERT_NEAR(res1.R23, 0.0f, 0.00001);

  ASSERT_NEAR(res1.R31, 0.0f, 0.00001);
  ASSERT_NEAR(res1.R32, 0.0f, 0.00001);
  ASSERT_NEAR(res1.R33, -1.0f, 0.00001);

  // rotate vector
  vec3d vecA(2.0f, 5.0f, 11.0f);
  vec3d res2 = mat.transform(vecA);
  ASSERT_NEAR(res2.x, vecA.z, 0.00001);
  ASSERT_NEAR(res2.y, vecA.y, 0.00001);
  ASSERT_NEAR(res2.z, -vecA.x, 0.00001);

  vecA = vec3d(-2.0f, -5.0f, -11.0f);
  res2 = mat.transform(vecA);
  ASSERT_NEAR(res2.x, vecA.z, 0.00001);
  ASSERT_NEAR(res2.y, vecA.y, 0.00001);
  ASSERT_NEAR(res2.z, -vecA.x, 0.00001);
}

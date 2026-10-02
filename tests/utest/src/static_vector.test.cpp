#include "src/system/common/static_vector.h"

#include <gtest/gtest.h>
#include <string>

namespace lampda::common {

class StaticVectorTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    // Setup before each test
  }

  void TearDown() override
  {
    // Cleanup after each test
  }
};

// ============================================================================
// STATIC_VECTOR TESTS
// ============================================================================

TEST_F(StaticVectorTest, DefaultConstructor)
{
  static_vector<int, 5> vec;
  EXPECT_EQ(0, vec.size());
  EXPECT_TRUE(vec.empty());
}

TEST_F(StaticVectorTest, PushBack)
{
  static_vector<int, 5> vec;
  vec.push_back(10);
  vec.push_back(20);
  vec.push_back(30);
  EXPECT_EQ(3, vec.size());
}

TEST_F(StaticVectorTest, ElementAccess)
{
  static_vector<int, 5> vec;
  vec.push_back(10);
  vec.push_back(20);
  EXPECT_EQ(20, vec[1]);
}

TEST_F(StaticVectorTest, PopBack)
{
  static_vector<int, 5> vec;
  vec.push_back(10);
  vec.push_back(20);
  vec.pop_back();
  EXPECT_EQ(1, vec.size());
}

TEST_F(StaticVectorTest, Clear)
{
  static_vector<int, 5> vec;
  vec.push_back(10);
  vec.push_back(20);
  vec.clear();
  EXPECT_EQ(0, vec.size());
  EXPECT_TRUE(vec.empty());
}

TEST_F(StaticVectorTest, Iteration)
{
  static_vector<int, 5> vec;
  vec.push_back(1);
  vec.push_back(2);
  vec.push_back(3);

  int sum = 0;
  for (int val: vec)
  {
    sum += val;
  }
  EXPECT_EQ(6, sum);
}

TEST_F(StaticVectorTest, FillConstructor)
{
  static_vector<int, 5> vec(3, 42);
  EXPECT_EQ(3, vec.size());
  EXPECT_EQ(42, vec[0]);
  EXPECT_EQ(42, vec[1]);
  EXPECT_EQ(42, vec[2]);
}

TEST_F(StaticVectorTest, CopyConstructor)
{
  static_vector<int, 5> vec1;
  vec1.push_back(10);
  vec1.push_back(20);

  static_vector<int, 5> vec2(vec1);
  EXPECT_EQ(2, vec2.size());
  EXPECT_EQ(10, vec2[0]);
  EXPECT_EQ(20, vec2[1]);
}

TEST_F(StaticVectorTest, Erase)
{
  static_vector<int, 5> vec;
  vec.push_back(1);
  vec.push_back(2);
  vec.push_back(3);

  vec.erase(vec.begin() + 1);
  EXPECT_EQ(2, vec.size());
  EXPECT_EQ(1, vec[0]);
  EXPECT_EQ(3, vec[1]);
}

TEST_F(StaticVectorTest, Insert)
{
  static_vector<int, 5> vec;
  vec.push_back(1);
  vec.push_back(3);

  vec.insert(vec.begin() + 1, 2);
  EXPECT_EQ(3, vec.size());
  EXPECT_EQ(1, vec[0]);
  EXPECT_EQ(2, vec[1]);
  EXPECT_EQ(3, vec[2]);
}

TEST_F(StaticVectorTest, MaxSize)
{
  static_vector<int, 10> vec;
  EXPECT_EQ(10, vec.max_size());
}

TEST_F(StaticVectorTest, Data)
{
  static_vector<int, 5> vec;
  vec.push_back(42);
  int* ptr = vec.data();
  EXPECT_EQ(42, *ptr);
}

} // namespace lampda::common

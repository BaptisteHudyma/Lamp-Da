/**
 * Test for the static_map implementation, by Claude Haiku 4.5
 */

#include "src/system/common/static_map.h"

#include <gtest/gtest.h>
#include <string>

using namespace std::string_literals;

namespace lampda::common {

// ============================================================================
// Test Fixture for static_map
// ============================================================================

class StaticMapTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    // Setup code if needed
  }

  void TearDown() override
  {
    // Cleanup code if needed
  }
};

// ============================================================================
// Basic Insertion Tests
// ============================================================================

TEST_F(StaticMapTest, InsertSingleElement)
{
  static_map<unsigned int, unsigned int, 64> map;

  auto [it, success] = map.insert({1, 100});

  EXPECT_TRUE(success);
  EXPECT_EQ(map.size(), 1);
  EXPECT_EQ(map.at(1), 100);
}

TEST_F(StaticMapTest, InsertMultipleElements)
{
  static_map<unsigned int, unsigned int, 64> map;

  auto [it1, success1] = map.insert({1, 100});
  auto [it2, success2] = map.insert({2, 200});
  auto [it3, success3] = map.insert({3, 300});

  EXPECT_TRUE(success1);
  EXPECT_TRUE(success2);
  EXPECT_TRUE(success3);
  EXPECT_EQ(map.size(), 3);
  EXPECT_EQ(map.at(1), 100);
  EXPECT_EQ(map.at(2), 200);
  EXPECT_EQ(map.at(3), 300);
}

TEST_F(StaticMapTest, InsertDuplicateKeyFails)
{
  static_map<unsigned int, unsigned int, 64> map;

  auto [it1, success1] = map.insert({1, 100});
  auto [it2, success2] = map.insert({1, 999}); // Duplicate key

  EXPECT_TRUE(success1);
  EXPECT_FALSE(success2);
  EXPECT_EQ(map.size(), 1);
  EXPECT_EQ(map.at(1), 100); // Original value preserved
}

TEST_F(StaticMapTest, InsertIntoFullMapFails)
{
  static_map<int, int, 3> small_map;

  auto [it1, success1] = small_map.insert({1, 10});
  auto [it2, success2] = small_map.insert({2, 20});
  auto [it3, success3] = small_map.insert({3, 30});
  auto [it4, success4] = small_map.insert({4, 40}); // Exceeds capacity

  EXPECT_TRUE(success1);
  EXPECT_TRUE(success2);
  EXPECT_TRUE(success3);
  EXPECT_FALSE(success4);
  EXPECT_EQ(small_map.size(), 3);
  EXPECT_TRUE(small_map.full());
}

// ============================================================================
// Finding and Existence Tests
// ============================================================================

TEST_F(StaticMapTest, FindExistingElement)
{
  static_map<int, const char*, 10> map;

  map.insert({1, "one"});
  map.insert({2, "two"});

  auto it = map.find(2);

  EXPECT_NE(it, map.end());
  EXPECT_EQ(it->second, "two"s);
}

TEST_F(StaticMapTest, FindNonExistentElement)
{
  static_map<int, const char*, 10> map;

  map.insert({1, "one"});

  auto it = map.find(999);

  EXPECT_EQ(it, map.end());
}

TEST_F(StaticMapTest, ContainsExistingKey)
{
  static_map<int, int, 10> map;

  map.insert({5, 50});

  EXPECT_TRUE(map.contains(5));
}

TEST_F(StaticMapTest, ContainsNonExistentKey)
{
  static_map<int, int, 10> map;

  map.insert({5, 50});

  EXPECT_FALSE(map.contains(999));
}

TEST_F(StaticMapTest, CountExistingKey)
{
  static_map<int, int, 10> map;

  map.insert({1, 10});

  EXPECT_EQ(map.count(1), 1);
}

TEST_F(StaticMapTest, CountNonExistentKey)
{
  static_map<int, int, 10> map;

  map.insert({1, 10});

  EXPECT_EQ(map.count(999), 0);
}

// ============================================================================
// Iteration Tests
// ============================================================================

TEST_F(StaticMapTest, ForwardIteration)
{
  static_map<char, int, 5> map;

  map.insert({'a', 1});
  map.insert({'b', 2});
  map.insert({'c', 3});

  int count = 0;
  for (auto it = map.begin(); it != map.end(); ++it)
  {
    EXPECT_TRUE(count >= 0 && count < 3);
    count++;
  }

  EXPECT_EQ(count, 3);
}

TEST_F(StaticMapTest, ReverseIteration)
{
  static_map<int, int, 5> map;

  map.insert({1, 10});
  map.insert({2, 20});
  map.insert({3, 30});

  int count = 0;
  for (auto it = map.rbegin(); it != map.rend(); ++it)
  {
    EXPECT_TRUE(count >= 0 && count < 3);
    count++;
  }

  EXPECT_EQ(count, 3);
}

TEST_F(StaticMapTest, RangeBasedForLoop)
{
  static_map<int, int, 4> map;

  map.insert({10, 100});
  map.insert({20, 200});
  map.insert({30, 300});

  int sum = 0;
  for (const auto& [key, value]: map)
  {
    sum += value;
  }

  EXPECT_EQ(sum, 600);
}

TEST_F(StaticMapTest, ConstIteration)
{
  static_map<int, int, 5> map;

  map.insert({1, 10});
  map.insert({2, 20});

  const auto& const_map = map;
  int count = 0;
  for (auto it = const_map.begin(); it != const_map.end(); ++it)
  {
    count++;
  }

  EXPECT_EQ(count, 2);
}

// ============================================================================
// Erase Tests
// ============================================================================

TEST_F(StaticMapTest, EraseByKey)
{
  static_map<int, int, 10> map;

  for (int i = 1; i <= 5; ++i)
  {
    map.insert({i, i * 10});
  }

  EXPECT_EQ(map.size(), 5);

  size_t erased = map.erase(3);

  EXPECT_EQ(erased, 1);
  EXPECT_EQ(map.size(), 4);
  EXPECT_FALSE(map.contains(3));
}

TEST_F(StaticMapTest, EraseNonExistentKey)
{
  static_map<int, int, 10> map;

  map.insert({1, 10});

  size_t erased = map.erase(999);

  EXPECT_EQ(erased, 0);
  EXPECT_EQ(map.size(), 1);
}

TEST_F(StaticMapTest, EraseByIterator)
{
  static_map<int, int, 10> map;

  map.insert({1, 10});
  map.insert({2, 20});
  map.insert({3, 30});

  auto it = map.find(2);
  auto next_it = map.erase(it);

  EXPECT_EQ(map.size(), 2);
  EXPECT_FALSE(map.contains(2));
}

// Disabled: erase uses swap-with-last, which is correct but changes physical order
TEST_F(StaticMapTest, EraseMultipleElements)
{
  static_map<int, int, 10> map;

  for (int i = 1; i <= 5; ++i)
  {
    map.insert({i, i * 10});
  }
  EXPECT_EQ(map.size(), 5);

  // Erase element and verify
  EXPECT_EQ(map.erase(1), 1);
  EXPECT_EQ(map.size(), 4);
  EXPECT_FALSE(map.contains(1));

  EXPECT_EQ(map.erase(3), 1);
  EXPECT_EQ(map.size(), 3);
  EXPECT_FALSE(map.contains(3));

  EXPECT_EQ(map.erase(5), 1);
  EXPECT_EQ(map.size(), 2);
  EXPECT_FALSE(map.contains(5));

  EXPECT_TRUE(map.contains(2));
  EXPECT_TRUE(map.contains(4));
}

// ============================================================================
// Insert or Assign Tests
// ============================================================================

TEST_F(StaticMapTest, InsertOrAssignNewKey)
{
  static_map<int, int, 5> map;

  auto [it, inserted] = map.insert_or_assign(1, 100);

  EXPECT_TRUE(inserted);
  EXPECT_EQ(map.size(), 1);
  EXPECT_EQ(map.at(1), 100);
}

TEST_F(StaticMapTest, InsertOrAssignExistingKey)
{
  static_map<int, int, 5> map;

  map.insert({1, 100});
  auto [it, inserted] = map.insert_or_assign(1, 999);

  EXPECT_FALSE(inserted);
  EXPECT_EQ(map.size(), 1);
  EXPECT_EQ(map.at(1), 999);
}

TEST_F(StaticMapTest, InsertOrAssignToFullMap)
{
  static_map<int, int, 2> map;

  map.insert({1, 10});
  map.insert({2, 20});

  auto [it, success] = map.insert_or_assign(3, 30);

  EXPECT_FALSE(success);
  EXPECT_EQ(map.size(), 2);
}

TEST_F(StaticMapTest, InsertOrAssignMultipleTimes)
{
  static_map<int, int, 5> map;

  map.insert_or_assign(1, 100);
  map.insert_or_assign(1, 200);
  map.insert_or_assign(1, 300);

  EXPECT_EQ(map.size(), 1);
  EXPECT_EQ(map.at(1), 300);
}

// ============================================================================
// Capacity and Status Tests
// ============================================================================

TEST_F(StaticMapTest, SizeTracking)
{
  static_map<int, int, 10> map;

  EXPECT_EQ(map.size(), 0);

  map.insert({1, 10});
  EXPECT_EQ(map.size(), 1);

  map.insert({2, 20});
  EXPECT_EQ(map.size(), 2);

  map.erase(1);
  EXPECT_EQ(map.size(), 1);
}

TEST_F(StaticMapTest, CapacityConstant)
{
  static_map<int, int, 64> map;

  EXPECT_EQ(map.capacity(), 64);
}

TEST_F(StaticMapTest, EmptyCheck)
{
  static_map<int, int, 5> map;

  EXPECT_TRUE(map.empty());

  map.insert({1, 10});
  EXPECT_FALSE(map.empty());

  map.erase(1);
  EXPECT_TRUE(map.empty());
}

TEST_F(StaticMapTest, FullCheck)
{
  static_map<int, int, 3> map;

  EXPECT_FALSE(map.full());

  map.insert({1, 10});
  EXPECT_FALSE(map.full());

  map.insert({2, 20});
  EXPECT_FALSE(map.full());

  map.insert({3, 30});
  EXPECT_TRUE(map.full());

  map.erase(1);
  EXPECT_FALSE(map.full());
}

// ============================================================================
// Clear Tests
// ============================================================================

TEST_F(StaticMapTest, ClearEmptiesMap)
{
  static_map<int, int, 5> map;

  map.insert({1, 10});
  map.insert({2, 20});
  map.insert({3, 30});

  EXPECT_EQ(map.size(), 3);

  map.clear();

  EXPECT_EQ(map.size(), 0);
  EXPECT_TRUE(map.empty());
  EXPECT_FALSE(map.contains(1));
}

TEST_F(StaticMapTest, ClearAllowsReinsertion)
{
  static_map<int, int, 3> map;

  map.insert({1, 10});
  map.insert({2, 20});
  map.insert({3, 30});

  map.clear();

  auto [it, success] = map.insert({1, 100});

  EXPECT_TRUE(success);
  EXPECT_EQ(map.size(), 1);
}

TEST_F(StaticMapTest, ClearOnEmptyMap)
{
  static_map<int, int, 5> map;

  EXPECT_TRUE(map.empty());

  map.clear();

  EXPECT_TRUE(map.empty());
  EXPECT_EQ(map.size(), 0);
}

// ============================================================================
// Emplace Tests
// ============================================================================

TEST_F(StaticMapTest, EmplaceNewElement)
{
  static_map<int, int, 5> map;

  auto [it, success] = map.emplace(1, 42);

  EXPECT_TRUE(success);
  EXPECT_EQ(map.size(), 1);
  EXPECT_EQ(map.at(1), 42);
}

TEST_F(StaticMapTest, EmplaceDuplicateKeyFails)
{
  static_map<int, int, 5> map;

  auto [it1, success1] = map.emplace(1, 42);
  auto [it2, success2] = map.emplace(1, 100);

  EXPECT_TRUE(success1);
  EXPECT_FALSE(success2);
  EXPECT_EQ(map.size(), 1);
  EXPECT_EQ(map.at(1), 42);
}

TEST_F(StaticMapTest, EmplaceIntoFullMapFails)
{
  static_map<int, int, 2> map;

  map.emplace(1, 10);
  map.emplace(2, 20);

  auto [it, success] = map.emplace(3, 30);

  EXPECT_FALSE(success);
  EXPECT_EQ(map.size(), 2);
}

// ============================================================================
// Element Access Tests
// ============================================================================

TEST_F(StaticMapTest, AtExistingKey)
{
  static_map<int, int, 5> map;

  map.insert({5, 50});

  int value = map.at(5);

  EXPECT_EQ(value, 50);
}

TEST_F(StaticMapTest, AtConstExistingKey)
{
  static_map<int, int, 5> map;

  map.insert({5, 50});

  const auto& const_map = map;
  int value = const_map.at(5);

  EXPECT_EQ(value, 50);
}

TEST_F(StaticMapTest, AtModifyValue)
{
  static_map<int, int, 5> map;

  map.insert({1, 100});

  map.at(1) = 999;

  EXPECT_EQ(map.at(1), 999);
}

// ============================================================================
// Different Types Tests
// ============================================================================

TEST_F(StaticMapTest, StringKeyIntValue)
{
  static_map<const char*, int, 5> map;

  map.insert({"apple", 5});
  map.insert({"banana", 3});

  EXPECT_EQ(map.at("apple"), 5);
  EXPECT_EQ(map.at("banana"), 3);
}

TEST_F(StaticMapTest, CharKeyIntValue)
{
  static_map<char, int, 10> map;

  map.insert({'a', 1});
  map.insert({'z', 26});

  EXPECT_EQ(map.at('a'), 1);
  EXPECT_EQ(map.at('z'), 26);
}

TEST_F(StaticMapTest, IntKeyStringValue)
{
  static_map<int, const char*, 5> map;

  map.insert({1, "one"});
  map.insert({2, "two"});

  EXPECT_EQ(map.at(1), "one"s);
  EXPECT_EQ(map.at(2), "two"s);
}

// ============================================================================
// Edge Cases and Stress Tests
// ============================================================================

TEST_F(StaticMapTest, InsertEraseInsertCycle)
{
  static_map<int, int, 5> map;

  // Insert
  map.insert({1, 10});
  map.insert({2, 20});
  EXPECT_EQ(map.size(), 2);

  // Erase
  map.erase(1);
  EXPECT_EQ(map.size(), 1);

  // Insert again
  auto [it, success] = map.insert({3, 30});
  EXPECT_TRUE(success);
  EXPECT_EQ(map.size(), 2);
}

TEST_F(StaticMapTest, InsertEraseInsertSameKey)
{
  static_map<int, int, 5> map;

  map.insert({1, 10});
  EXPECT_EQ(map.at(1), 10);

  map.erase(1);
  EXPECT_FALSE(map.contains(1));

  auto [it, success] = map.insert({1, 20});
  EXPECT_TRUE(success);
  EXPECT_EQ(map.at(1), 20);
}

// Disabled: erase uses swap-with-last, which is correct but changes physical order
TEST_F(StaticMapTest, ContinuousInsertAndErase)
{
  static_map<int, int, 10> map;

  for (int i = 0; i < 10; ++i)
  {
    auto [it, s] = map.insert({i, i * 10});
    EXPECT_TRUE(s);
  }
  EXPECT_EQ(map.size(), 10);

  for (int i = 0; i < 10; i += 2)
  {
    size_t erased = map.erase(i);
    EXPECT_EQ(erased, 1);
  }
  EXPECT_EQ(map.size(), 5);

  EXPECT_TRUE(map.contains(1));
  EXPECT_TRUE(map.contains(3));
  EXPECT_TRUE(map.contains(5));
  EXPECT_TRUE(map.contains(7));
  EXPECT_TRUE(map.contains(9));
}

TEST_F(StaticMapTest, FillAndClearMultipleTimes)
{
  static_map<int, int, 3> map;

  for (int cycle = 0; cycle < 5; ++cycle)
  {
    map.insert({1, 10});
    map.insert({2, 20});
    map.insert({3, 30});

    EXPECT_EQ(map.size(), 3);
    EXPECT_TRUE(map.full());

    map.clear();

    EXPECT_EQ(map.size(), 0);
    EXPECT_TRUE(map.empty());
  }
}

// ============================================================================
// Iterator Validity Tests
// ============================================================================

TEST_F(StaticMapTest, IteratorEquality)
{
  static_map<int, int, 5> map;

  map.insert({1, 10});

  auto it1 = map.find(1);
  auto it2 = map.find(1);

  EXPECT_EQ(it1, it2);
}

TEST_F(StaticMapTest, EndIteratorComparison)
{
  static_map<int, int, 5> map;

  // Empty map case
  auto it_empty = map.find(999);
  EXPECT_EQ(it_empty, map.end());

  // Non-empty map case
  map.insert({1, 10});
  auto it = map.find(999);

  EXPECT_EQ(it, map.end());
  // With an element, begin() != end()
  EXPECT_NE(map.begin(), map.end());
}

// ============================================================================
// Const Map Tests
// ============================================================================

TEST_F(StaticMapTest, ConstMapAccess)
{
  static_map<int, int, 5> map;

  map.insert({1, 100});
  map.insert({2, 200});

  const auto& const_map = map;

  EXPECT_EQ(const_map.size(), 2);
  EXPECT_TRUE(const_map.contains(1));
  EXPECT_EQ(const_map.at(1), 100);
  EXPECT_EQ(const_map.find(2)->second, 200);
}

TEST_F(StaticMapTest, ConstMapIteration)
{
  static_map<int, int, 5> map;

  map.insert({1, 10});
  map.insert({2, 20});
  map.insert({3, 30});

  const auto& const_map = map;

  int count = 0;
  int sum = 0;
  for (const auto& [k, v]: const_map)
  {
    count++;
    sum += v;
  }

  EXPECT_EQ(count, 3);
  EXPECT_EQ(sum, 60);
}

} // namespace lampda::common

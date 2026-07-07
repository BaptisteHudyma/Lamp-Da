#include <cmath>
#include <cstdint>
#include <gtest/gtest.h>
#include <limits>
#include "src/system/utils/queue.h"

namespace lampda::utils {

TEST(test_queue, simple_value_queue)
{
  static constexpr size_t elementMaxCnt = 5;
  Queue<uint8_t, elementMaxCnt> queue;

  ASSERT_FALSE(queue.has_elements());
  ASSERT_FALSE(queue.dequeue().has_value());

  // can add up to N elements
  for (uint8_t i = 0; i < elementMaxCnt; i++)
  {
    ASSERT_TRUE(queue.enqueue_element(i));
    ASSERT_TRUE(queue.has_elements());
  }
  // N+1 cannot be added
  ASSERT_FALSE(queue.enqueue_element(elementMaxCnt));
  ASSERT_TRUE(queue.has_elements());

  // can dequeue N elements, in order of insertion
  for (uint8_t i = 0; i < elementMaxCnt; i++)
  {
    ASSERT_TRUE(queue.has_elements());
    const auto& element = queue.dequeue();
    ASSERT_TRUE(element.has_value());
    ASSERT_EQ(element.value(), i);
  }

  // queue is empty
  ASSERT_FALSE(queue.has_elements());
  const auto& element = queue.dequeue();
  ASSERT_FALSE(element.has_value());
}

TEST(test_queue, complex_enqueue_dequeue)
{
  static constexpr size_t elementMaxCnt = 15;
  Queue<uint8_t, elementMaxCnt> queue;

  ASSERT_FALSE(queue.has_elements());
  ASSERT_FALSE(queue.dequeue().has_value());

  // can add 10 elements
  for (uint8_t i = 0; i < 10; i++)
  {
    ASSERT_TRUE(queue.enqueue_element(i));
    ASSERT_TRUE(queue.has_elements());
  }

  // can remove the first 5
  for (uint8_t i = 0; i < 5; i++)
  {
    ASSERT_TRUE(queue.has_elements());
    const auto& element = queue.dequeue();
    ASSERT_TRUE(element.has_value());
    ASSERT_EQ(element.value(), i);
  }

  // can add 7
  for (uint8_t i = 0; i < 7; i++)
  {
    ASSERT_TRUE(queue.enqueue_element(i));
    ASSERT_TRUE(queue.has_elements());
  }

  // can remove the first 5
  for (uint8_t i = 0; i < 5; i++)
  {
    ASSERT_TRUE(queue.has_elements());
    const auto& element = queue.dequeue();
    ASSERT_TRUE(element.has_value());
    ASSERT_EQ(element.value(), i + 5);
  }
  // And then the other 7
  for (uint8_t i = 0; i < 7; i++)
  {
    ASSERT_TRUE(queue.has_elements());
    const auto& element = queue.dequeue();
    ASSERT_TRUE(element.has_value());
    ASSERT_EQ(element.value(), i);
  }

  ASSERT_FALSE(queue.has_elements());
}

} // namespace lampda::utils

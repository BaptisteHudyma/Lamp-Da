#include <gtest/gtest.h>

#include <atomic>
#include <cstdint>
#include <thread>

#include "src/system/logic/inputs.h"

namespace lampda::logic {

namespace {

namespace queue = inputs::__private;

static constexpr size_t queueCapacity = queue::maxButtonEventStore;

} // namespace

class button_event_queue_test : public ::testing::Test
{
protected:
  void SetUp() override { queue::clear_button_events(); }
  void TearDown() override { queue::clear_button_events(); }
};

TEST_F(button_event_queue_test, pop_on_empty_queue)
{
  queue::ButtonEvent event;
  EXPECT_FALSE(queue::pop_button_event(event));
  EXPECT_EQ(queue::get_button_event_count(), 0u);
}

TEST_F(button_event_queue_test, events_kept_in_order)
{
  ASSERT_TRUE(inputs::add_button_click_event(2));
  ASSERT_TRUE(inputs::add_button_press_event(3, 600, false));
  ASSERT_TRUE(inputs::add_button_press_event(3, 700, true));

  queue::ButtonEvent event;
  ASSERT_TRUE(queue::pop_button_event(event));
  EXPECT_FALSE(event.isLongPress);
  EXPECT_EQ(event.clickCount, 2);

  ASSERT_TRUE(queue::pop_button_event(event));
  EXPECT_TRUE(event.isLongPress);
  EXPECT_FALSE(event.isEndOfLongPress);
  EXPECT_EQ(event.clickCount, 3);
  EXPECT_EQ(event.longPressDuration, 600u);

  ASSERT_TRUE(queue::pop_button_event(event));
  EXPECT_TRUE(event.isEndOfLongPress);
  EXPECT_EQ(event.longPressDuration, 700u);

  EXPECT_FALSE(queue::pop_button_event(event));
}

/// Hold events stop at 80% of the queue, to keep room for the end of hold and the clicks
TEST_F(button_event_queue_test, hold_events_capped)
{
  size_t accepted = 0;
  while (inputs::add_button_press_event(1, 600, false))
  {
    ++accepted;
    ASSERT_LE(accepted, queueCapacity);
  }
  EXPECT_EQ(accepted, (queueCapacity * 80 + 99) / 100);

  // the end of hold still has room
  EXPECT_TRUE(inputs::add_button_press_event(1, 600, true));
}

/// Issue #602: an end of hold pushed into a full queue was not checked (assert in etl::queue)
TEST_F(button_event_queue_test, end_of_hold_refused_on_full_queue)
{
  for (size_t i = 0; i < queueCapacity; ++i)
    ASSERT_TRUE(inputs::add_button_click_event(1)) << "event " << i;
  ASSERT_EQ(queue::get_button_event_count(), queueCapacity);

  EXPECT_FALSE(inputs::add_button_click_event(1));
  EXPECT_FALSE(inputs::add_button_press_event(1, 600, false));
  EXPECT_FALSE(inputs::add_button_press_event(1, 600, true));
  EXPECT_EQ(queue::get_button_event_count(), queueCapacity);

  // the queue is still usable
  queue::ButtonEvent event;
  ASSERT_TRUE(queue::pop_button_event(event));
  EXPECT_TRUE(inputs::add_button_press_event(4, 600, true));
}

/// Issue #602: the button thread pushes while the main loop pops. No event may be lost, duplicated or reordered
TEST_F(button_event_queue_test, concurrent_push_and_pop)
{
  static constexpr uint32_t eventCount = 20000;

  std::atomic<bool> isProducerDone {false};
  std::thread producer([&]() {
    for (uint32_t i = 0; i < eventCount;)
    {
      // the sequence number is carried by the duration, the click count checks the event integrity
      if (inputs::add_button_press_event(static_cast<uint8_t>(i), i, true))
        ++i;
      else
        std::this_thread::yield();
    }
    isProducerDone = true;
  });

  uint32_t expected = 0;
  bool isOk = true;
  while (expected < eventCount and isOk)
  {
    queue::ButtonEvent event;
    if (not queue::pop_button_event(event))
    {
      if (isProducerDone and queue::get_button_event_count() == 0)
        break;
      std::this_thread::yield();
      continue;
    }

    isOk = event.isLongPress and event.isEndOfLongPress and event.longPressDuration == expected and
           event.clickCount == static_cast<uint8_t>(expected);
    EXPECT_TRUE(isOk) << "event " << expected << " received as " << event.longPressDuration;
    ++expected;
  }
  producer.join();

  EXPECT_EQ(expected, eventCount);
  EXPECT_EQ(queue::get_button_event_count(), 0u);
}

} // namespace lampda::logic

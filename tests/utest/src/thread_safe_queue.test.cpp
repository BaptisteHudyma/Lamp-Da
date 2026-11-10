#include <gtest/gtest.h>

#include <atomic>
#include <cstdint>
#include <thread>

#include "src/system/common/user_commands.h"
#include "src/system/logic/user_command_queue.h"
#include "src/system/utils/thread_safe_queue.h"

namespace lampda {

TEST(thread_safe_queue, fifo_order)
{
  utils::ThreadSafeQueue<int, 4> queue;
  EXPECT_FALSE(queue.pop().has_value());

  EXPECT_TRUE(queue.push(1));
  EXPECT_TRUE(queue.push(2));
  EXPECT_TRUE(queue.push(3));
  EXPECT_EQ(queue.size(), 3u);

  EXPECT_EQ(queue.pop().value(), 1);
  EXPECT_EQ(queue.pop().value(), 2);
  EXPECT_EQ(queue.pop().value(), 3);
  EXPECT_FALSE(queue.pop().has_value());
}

TEST(thread_safe_queue, full_queue_drops)
{
  utils::ThreadSafeQueue<int, 2> queue;
  EXPECT_TRUE(queue.push(1));
  EXPECT_TRUE(queue.push(2));
  EXPECT_FALSE(queue.push(3));
  EXPECT_EQ(queue.size(), 2u);

  queue.clear();
  EXPECT_EQ(queue.size(), 0u);
  EXPECT_TRUE(queue.push(4));
  EXPECT_EQ(queue.pop().value(), 4);
}

TEST(thread_safe_queue, concurrent_producers_lose_nothing)
{
  static constexpr int itemsPerProducer = 2000;
  utils::ThreadSafeQueue<int, 8> queue;

  std::atomic<int> pushed {0};
  auto producer = [&queue, &pushed]() {
    for (int i = 0; i < itemsPerProducer; ++i)
    {
      while (not queue.push(i))
        std::this_thread::yield();
      pushed++;
    }
  };

  std::thread first(producer);
  std::thread second(producer);

  int popped = 0;
  int64_t sum = 0;
  while (popped < 2 * itemsPerProducer)
  {
    const auto item = queue.pop();
    if (item.has_value())
    {
      popped++;
      sum += item.value();
    }
    else
      std::this_thread::yield();
  }
  first.join();
  second.join();

  EXPECT_EQ(pushed.load(), 2 * itemsPerProducer);
  EXPECT_EQ(sum, 2 * static_cast<int64_t>(itemsPerProducer) * (itemsPerProducer - 1) / 2);
  EXPECT_FALSE(queue.pop().has_value());
}

TEST(user_command_queue, posted_commands_wait_for_the_main_loop)
{
  // drop leftovers of other tests
  logic::user_command_queue::loop();
  ASSERT_EQ(logic::user_command_queue::get_pending_count(), 0u);

  EXPECT_TRUE(logic::user_command_queue::post(common::UserCommand::make_signal_suntimer_cancel_command()));
  EXPECT_TRUE(logic::user_command_queue::post(common::UserCommand::make_signal_suntimer_cancel_command()));
  EXPECT_EQ(logic::user_command_queue::get_pending_count(), 2u);

  logic::user_command_queue::loop();
  EXPECT_EQ(logic::user_command_queue::get_pending_count(), 0u);
}

TEST(user_command_queue, full_queue_drops)
{
  logic::user_command_queue::loop();

  for (size_t i = 0; i < logic::user_command_queue::maxStoredCommands; ++i)
    EXPECT_TRUE(logic::user_command_queue::post(common::UserCommand::make_signal_suntimer_cancel_command()));
  EXPECT_FALSE(logic::user_command_queue::post(common::UserCommand::make_signal_suntimer_cancel_command()));

  logic::user_command_queue::loop();
  EXPECT_EQ(logic::user_command_queue::get_pending_count(), 0u);
}

} // namespace lampda

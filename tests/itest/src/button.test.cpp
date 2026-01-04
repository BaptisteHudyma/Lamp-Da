#include <gtest/gtest.h>
#include <thread>
#include <chrono>

#include "src/system/physical/button.h"

#include "src/system/platform/gpio.h"

// access simulation states
#include "simulator/include/simulator_state.h"
#include "simulator/include/hardware_influencer.h"

using namespace std::chrono_literals;

// 10 seconds timeout
static constexpr uint32_t testTimeout_ms = 10 * 1000;

class ButtonFixture : public ::testing::Test
{
protected:
  void SetUp() override
  {
    //
    killThread = false;
    time_mocks::reset();
  }

  void TearDown() override
  {
    killThread = true;
    if (clickThread.joinable())
      clickThread.join();
  }

  // start a click simulating thread
  void simulate_clicks(const int clicks,
                       const std::chrono::milliseconds holdPress = 200ms,
                       const std::chrono::milliseconds releasePress = 100ms,
                       const std::chrono::milliseconds startDelay = 0ms,
                       const std::chrono::milliseconds lastClickPressDelay = 0ms)
  {
    clickThread = std::thread([&]() {
      int remainingClicks = clicks;
      if (lastClickPressDelay > 0ms)
        remainingClicks--;

      if (startDelay > 0ms)
        std::this_thread::sleep_for(startDelay);

      while (remainingClicks > 0 && not killThread)
      {
        sim::globals::state.isButtonPressed = true;
        mock_gpios::update_callbacks();
        std::this_thread::sleep_for(holdPress);

        sim::globals::state.isButtonPressed = false;
        mock_gpios::update_callbacks();
        std::this_thread::sleep_for(releasePress);

        remainingClicks--;
      };

      if (not killThread && lastClickPressDelay > 0ms)
      {
        sim::globals::state.isButtonPressed = true;
        mock_gpios::update_callbacks();
        std::this_thread::sleep_for(lastClickPressDelay);

        sim::globals::state.isButtonPressed = false;
        mock_gpios::update_callbacks();
      }
    });
  }

private:
  bool killThread = false;
  std::thread clickThread;
};

/**
 * Check system start click
 */

// Start system click, with no button click detected
TEST_F(ButtonFixture, turn_on_start_click_early_release)
{
  // initial state is turn on
  sim::globals::state.isButtonPressed = false;
  bool isTestDone = false;

  auto clickSerieCallback = [&](uint8_t clickCount) {
    isTestDone = true;
    ASSERT_EQ(clickCount, 1);
    ASSERT_TRUE(button::is_system_start_click());
  };
  auto clickHoldSerieCallback = [&](uint8_t clickCount, uint32_t clickDuration) {
    isTestDone = true;
    ADD_FAILURE() << "long click when a short click serie was expected";
  };

  // signal button on
  mock_gpios::update_callbacks();
  button::init(true);

  while (time_ms() < testTimeout_ms && not isTestDone)
  {
    mock_gpios::update_callbacks();
    std::this_thread::sleep_for(10ms);
    button::handle_events(clickSerieCallback, clickHoldSerieCallback);
  }
  ASSERT_TRUE(isTestDone);
}

// Start system click, with button click detected
TEST_F(ButtonFixture, turn_on_start_click)
{
  // initial state is turn on
  sim::globals::state.isButtonPressed = true;
  bool isTestDone = false;

  auto clickSerieCallback = [&](uint8_t clickCount) {
    isTestDone = true;
    ASSERT_EQ(clickCount, 1);
    ASSERT_TRUE(button::is_system_start_click());
  };
  auto clickHoldSerieCallback = [&](uint8_t clickCount, uint32_t clickDuration) {
    isTestDone = true;
    ADD_FAILURE() << "long click when a short click serie was expected";
  };

  // signal button on
  mock_gpios::update_callbacks();
  button::init(true);

  // simulate clicks
  auto buttonThread = std::thread([]() {
    std::this_thread::sleep_for(150ms);
    sim::globals::state.isButtonPressed = false;
    mock_gpios::update_callbacks();
  });

  while (time_ms() < testTimeout_ms && not isTestDone)
  {
    mock_gpios::update_callbacks();
    std::this_thread::sleep_for(10ms);
    button::handle_events(clickSerieCallback, clickHoldSerieCallback);
  }
  ASSERT_TRUE(isTestDone);
  buttonThread.join();
}

// Start system click, with multiple button clicks detected
TEST_F(ButtonFixture, turn_on_start_multiple_clicks)
{
  static const int desiredClicks = 4;

  // initial state is turn on
  sim::globals::state.isButtonPressed = true;
  std::atomic<bool> isTestDone = false;

  auto clickSerieCallback = [&](uint8_t clickCount) {
    isTestDone = true;
    ASSERT_EQ(clickCount, desiredClicks);
    ASSERT_TRUE(button::is_system_start_click());
  };
  auto clickHoldSerieCallback = [&](uint8_t clickCount, uint32_t clickDuration) {
    isTestDone = true;
    ADD_FAILURE() << "long click when a short click serie was expected";
  };

  // signal button on
  mock_gpios::update_callbacks();
  button::init(true);

  // simulate click release
  auto buttonThread = std::thread([&]() {
    std::this_thread::sleep_for(50ms);
    sim::globals::state.isButtonPressed = false;
    mock_gpios::update_callbacks();
  });

  // simulate a few clicks
  simulate_clicks(desiredClicks - 1, 200ms, 100ms, 200ms);

  while (time_ms() < testTimeout_ms && not isTestDone)
  {
    std::this_thread::sleep_for(10ms);
    button::handle_events(clickSerieCallback, clickHoldSerieCallback);
  }
  ASSERT_TRUE(isTestDone);
  buttonThread.join();
}

// Start system click, with long button click detected
TEST_F(ButtonFixture, turn_on_start_long_click)
{
  // initial state is turn on
  sim::globals::state.isButtonPressed = true;
  bool isTestDone = false;

  auto clickSerieCallback = [&](uint8_t clickCount) {
    isTestDone = true;
    ADD_FAILURE() << "short click when a long click was expected";
  };
  auto clickHoldSerieCallback = [&](uint8_t clickCount, uint32_t clickDuration) {
    if (clickDuration > 0)
    {
      EXPECT_EQ(clickCount, 1);
      EXPECT_LT(clickDuration, 1000 * 1.5);
      EXPECT_TRUE(button::is_system_start_click());
    }
    else
    {
      EXPECT_EQ(clickCount, 1);
      // end of long click is always zero duration
      EXPECT_EQ(clickDuration, 0);
      EXPECT_TRUE(button::is_system_start_click());
      isTestDone = true;
    }
  };

  // signal button on
  mock_gpios::update_callbacks();
  button::init(true);

  // simulate click release after a long time
  auto buttonThread = std::thread([]() {
    std::this_thread::sleep_for(1000ms);
    sim::globals::state.isButtonPressed = false;
    mock_gpios::update_callbacks();
  });

  while (time_ms() < testTimeout_ms && not isTestDone)
  {
    mock_gpios::update_callbacks();
    std::this_thread::sleep_for(10ms);
    button::handle_events(clickSerieCallback, clickHoldSerieCallback);
  }
  ASSERT_TRUE(isTestDone);
  buttonThread.join();
}

// Start system click, with long button click detected
TEST_F(ButtonFixture, turn_on_start_multiple_long_clicks)
{
  static const int desiredClicks = 4;

  // initial state is turn on
  sim::globals::state.isButtonPressed = true;
  bool isTestDone = false;

  auto clickSerieCallback = [&](uint8_t clickCount) {
    isTestDone = true;
    ADD_FAILURE() << "short click when a long click was expected";
  };
  auto clickHoldSerieCallback = [&](uint8_t clickCount, uint32_t clickDuration) {
    if (clickDuration > 0)
    {
      EXPECT_EQ(clickCount, desiredClicks);
      EXPECT_LT(clickDuration, 1000 * 1.5);
      EXPECT_TRUE(button::is_system_start_click());
    }
    else
    {
      EXPECT_EQ(clickCount, desiredClicks);
      // end of long click is always zero duration
      EXPECT_EQ(clickDuration, 0);
      EXPECT_TRUE(button::is_system_start_click());
      isTestDone = true;
    }
  };

  // signal button on
  mock_gpios::update_callbacks();
  button::init(true);

  // simulate click release after a long time
  auto buttonThread = std::thread([&]() {
    std::this_thread::sleep_for(50ms);
    sim::globals::state.isButtonPressed = false;
    mock_gpios::update_callbacks();
  });

  // simulate a few clicks
  simulate_clicks(desiredClicks - 1, 200ms, 100ms, 200ms, 1000ms);

  while (time_ms() < testTimeout_ms && not isTestDone)
  {
    mock_gpios::update_callbacks();
    std::this_thread::sleep_for(10ms);
    button::handle_events(clickSerieCallback, clickHoldSerieCallback);
  }
  ASSERT_TRUE(isTestDone);
  buttonThread.join();
}

/**
 * Test standard button clicks
 */

// Check debouncing
TEST_F(ButtonFixture, debounce)
{
  static const int desiredClicks = 50;

  // initial state is turn on
  sim::globals::state.isButtonPressed = false;
  std::atomic<bool> isTestDone = false;

  auto clickSerieCallback = [&](uint8_t clickCount) {
    ADD_FAILURE() << "short click when a long click serie was expected";
    isTestDone = true;
  };
  auto clickHoldSerieCallback = [&](uint8_t clickCount, uint32_t clickDuration) {
    // only one click count, other are debounced
    ASSERT_EQ(clickCount, 1);
    ASSERT_TRUE(button::is_system_start_click());

    // test is done when all clicks are done
    if (clickDuration <= 0)
      isTestDone = true;
  };

  // signal button on
  mock_gpios::update_callbacks();
  button::init(true);

  // simulate a clicks
  simulate_clicks(desiredClicks, 15ms, 15ms);

  while (time_ms() < testTimeout_ms && not isTestDone)
  {
    std::this_thread::sleep_for(10ms);
    button::handle_events(clickSerieCallback, clickHoldSerieCallback);
  }
  ASSERT_TRUE(isTestDone);
}

TEST_F(ButtonFixture, standard_multiple_click)
{
  static const int desiredClicks = 4;

  // initial state is turn on
  sim::globals::state.isButtonPressed = true;
  std::atomic<bool> isTestDone = false;

  bool isFirstClick = true;

  auto clickSerieCallback = [&](uint8_t clickCount) {
    if (isFirstClick)
    {
      ASSERT_TRUE(button::is_system_start_click());
      ASSERT_EQ(clickCount, 1);
      isFirstClick = false;
    }
    else
    {
      ASSERT_FALSE(button::is_system_start_click());
      ASSERT_EQ(clickCount, desiredClicks);
      isTestDone = true;
    }
  };
  auto clickHoldSerieCallback = [&](uint8_t clickCount, uint32_t clickDuration) {
    isTestDone = true;
    ADD_FAILURE() << "long click when a short click serie was expected";
  };

  // signal button on
  mock_gpios::update_callbacks();
  button::init(true);

  // simulate click release
  auto buttonThread = std::thread([&]() {
    std::this_thread::sleep_for(50ms);
    sim::globals::state.isButtonPressed = false;
    mock_gpios::update_callbacks();
  });

  // simulate a few clicks
  simulate_clicks(desiredClicks, 200ms, 100ms, 500ms);

  while (time_ms() < testTimeout_ms && not isTestDone)
  {
    std::this_thread::sleep_for(10ms);
    button::handle_events(clickSerieCallback, clickHoldSerieCallback);
  }
  ASSERT_TRUE(isTestDone);
  buttonThread.join();
}

// TODO:
// standard click than long click chain
// long click start then click chain
// start click chain, then multiple click chains

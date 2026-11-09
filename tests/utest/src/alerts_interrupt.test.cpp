#include <gtest/gtest.h>

#include <atomic>
#include <thread>

#include "src/system/hal/time.h"

#include "src/system/logic/alerts.h"

// access simulation states
#include "simulator/include/hardware_influencer.h"

namespace lampda::logic {

class alerts_interrupt_test : public ::testing::Test
{
protected:
  void SetUp() override { reset(); }
  void TearDown() override { reset(); }

  static void reset()
  {
    // drop a signal left by another test
    alerts::process_interrupt_signals();
    alerts::manager.clear(alerts::Type::USB_PORT_SHORT);
  }
};

/// The interrupt only signals the fault: the alert is raised by the next processing, from a thread
TEST_F(alerts_interrupt_test, usb_fault_raised_on_processing)
{
  alerts::signal_usb_port_fault();
  EXPECT_FALSE(alerts::manager.is_raised(alerts::Type::USB_PORT_SHORT));

  alerts::process_interrupt_signals();
  EXPECT_TRUE(alerts::manager.is_raised(alerts::Type::USB_PORT_SHORT));
  EXPECT_FALSE(alerts::manager.can_use_usb_port());
  EXPECT_FALSE(alerts::manager.can_charge_battery());
}

/// No signal, no alert
TEST_F(alerts_interrupt_test, no_signal_no_alert)
{
  alerts::process_interrupt_signals();
  EXPECT_FALSE(alerts::manager.is_raised(alerts::Type::USB_PORT_SHORT));
}

/// A signal is consumed once
TEST_F(alerts_interrupt_test, signal_consumed_once)
{
  alerts::signal_usb_port_fault();
  alerts::signal_usb_port_fault();
  alerts::process_interrupt_signals();
  ASSERT_TRUE(alerts::manager.is_raised(alerts::Type::USB_PORT_SHORT));

  alerts::manager.clear(alerts::Type::USB_PORT_SHORT);
  alerts::process_interrupt_signals();
  EXPECT_FALSE(alerts::manager.is_raised(alerts::Type::USB_PORT_SHORT));
}

/// A fault signaled while the alert is raised refreshes its raise time (it is cleared after a delay)
TEST_F(alerts_interrupt_test, usb_fault_refreshes_raised_alert)
{
  // the raise time is set once the alerts loop handled the raise
  alerts::signal_usb_port_fault();
  alerts::handle_all(true);
  ASSERT_TRUE(alerts::manager.is_raised(alerts::Type::USB_PORT_SHORT));

  hal::delay_ms(100);
  alerts::handle_all(true);
  ASSERT_TRUE(alerts::manager.is_raised(alerts::Type::USB_PORT_SHORT));
  ASSERT_GE(alerts::manager.get_time_since_raised(alerts::Type::USB_PORT_SHORT), 100u);

  alerts::signal_usb_port_fault();
  alerts::process_interrupt_signals();
  EXPECT_TRUE(alerts::manager.is_raised(alerts::Type::USB_PORT_SHORT));
  EXPECT_LT(alerts::manager.get_time_since_raised(alerts::Type::USB_PORT_SHORT), 100u);
}

/// The alerts loop processes the signals
TEST_F(alerts_interrupt_test, usb_fault_raised_by_alerts_loop)
{
  alerts::signal_usb_port_fault();
  alerts::handle_all(true);
  EXPECT_TRUE(alerts::manager.is_raised(alerts::Type::USB_PORT_SHORT));
}

/// Signals from an interrupt-like context while two threads process them: the alert is always raised in the end
TEST_F(alerts_interrupt_test, concurrent_signal_and_processing)
{
  std::atomic<bool> shouldStop {false};
  std::thread processor([&]() {
    while (not shouldStop)
      alerts::process_interrupt_signals();
  });

  for (int i = 0; i < 2000; ++i)
  {
    alerts::manager.clear(alerts::Type::USB_PORT_SHORT);
    alerts::signal_usb_port_fault();
    // the signal is processed by one of the two threads: the other one may still be raising it
    alerts::process_interrupt_signals();
    const uint32_t start_ms = hal::time_ms();
    while (not alerts::manager.is_raised(alerts::Type::USB_PORT_SHORT))
    {
      ASSERT_LT(hal::time_ms() - start_ms, 1000u) << "signal lost, iteration " << i;
      std::this_thread::yield();
    }
  }

  shouldStop = true;
  processor.join();
}

} // namespace lampda::logic

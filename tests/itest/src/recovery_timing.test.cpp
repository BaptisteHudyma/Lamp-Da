#include <gtest/gtest.h>

#include "src/system/common/recovery_timing.h"
#include "src/system/common/state_machine.h"

using lampda::common::RecoveryTiming;

TEST(test_recovery_timing, recovery_timing)
{
  ASSERT_EQ(RecoveryTiming::startup_timeout_ms, 3000);
  ASSERT_EQ(RecoveryTiming::probe_timeout_ms(false), 900);
  ASSERT_EQ(RecoveryTiming::probe_timeout_ms(true), 2400);

  ASSERT_FALSE(RecoveryTiming::probe_timed_out(900, false));
  ASSERT_TRUE(RecoveryTiming::probe_timed_out(901, false));
  // The observed failure at 1047 ms should no longer reject an active recovery.
  ASSERT_TRUE(RecoveryTiming::probe_timed_out(1047, false));

  ASSERT_FALSE(RecoveryTiming::probe_timed_out(1047, true));
  ASSERT_FALSE(RecoveryTiming::probe_timed_out(2399, true));
  ASSERT_FALSE(RecoveryTiming::probe_timed_out(2400, true));
  ASSERT_TRUE(RecoveryTiming::probe_timed_out(2401, true));

  ASSERT_TRUE(RecoveryTiming::probe_timed_out(3000, true));
  ASSERT_EQ(RecoveryTiming::balancer_settle_ms, 250);
  ASSERT_LT(RecoveryTiming::recovery_ready_ms + RecoveryTiming::balancer_settle_ms, RecoveryTiming::startup_timeout_ms);
}

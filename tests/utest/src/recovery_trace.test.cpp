#include <gtest/gtest.h>

#include "src/system/common/recovery_trace.h"

using lampda::common::RecoveryStage;
using lampda::common::RecoveryTrace;

TEST(recovery_trace_test, recovery_trace)
{
  RecoveryTrace trace;
  ASSERT_EQ(trace.size(), 0);

  trace.probe(RecoveryStage::ChargerProbe, 2, 10);
  trace.probe(RecoveryStage::ChargerProbe, 2, 11);
  ASSERT_EQ(trace.charger_probes(), 2);
  ASSERT_EQ(trace.size(), 1);

  trace.once(RecoveryStage::VbusGateRequested, 0, 12);
  trace.once(RecoveryStage::VbusGateRequested, 0, 13);
  ASSERT_EQ(trace.event(1).time_ms, 12);
  ASSERT_EQ(trace.size(), 2);

  trace.probe(RecoveryStage::ChargerProbe, 0, 14);
  trace.probe(RecoveryStage::BalancerProbe, 0, 15);
  ASSERT_EQ(trace.size(), 4);
  ASSERT_EQ(trace.balancer_probes(), 1);

  ASSERT_EQ(trace.event(2).result, 0);
  ASSERT_EQ(trace.event(0).result, 2);

  for (unsigned i = 0; i < 100; ++i)
    trace.probe(RecoveryStage::ChargerProbe, i % 2, 20 + i);

  ASSERT_EQ(trace.size(), RecoveryTrace::capacity);
  ASSERT_GT(trace.omitted(), 0);
  ASSERT_EQ(trace.event(0).time_ms, 10);
  ASSERT_EQ(trace.charger_probes(), 103);
}

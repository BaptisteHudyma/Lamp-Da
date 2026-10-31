#include "src/system/common/recovery_trace.h"
#include <cassert>
#include <iostream>

using lampda::common::RecoveryStage;
using lampda::common::RecoveryTrace;

int main()
{
  RecoveryTrace trace;
  assert(trace.size() == 0);
  trace.probe(RecoveryStage::ChargerProbe, 2, 10);
  trace.probe(RecoveryStage::ChargerProbe, 2, 11);
  assert(trace.charger_probes() == 2 && trace.size() == 1);
  trace.once(RecoveryStage::VbusGateRequested, 0, 12);
  trace.once(RecoveryStage::VbusGateRequested, 0, 13);
  assert(trace.size() == 2 && trace.event(1).time_ms == 12);
  trace.probe(RecoveryStage::ChargerProbe, 0, 14);
  trace.probe(RecoveryStage::BalancerProbe, 0, 15);
  assert(trace.size() == 4 && trace.balancer_probes() == 1);
  assert(trace.event(2).result == 0 && trace.event(0).result == 2);
  for (unsigned i = 0; i < 100; ++i)
    trace.probe(RecoveryStage::ChargerProbe, i % 2, 20 + i);
  assert(trace.size() == RecoveryTrace::capacity);
  assert(trace.omitted() > 0);
  assert(trace.event(0).time_ms == 10);
  assert(trace.charger_probes() == 103);
  std::cout << "PASS: probe counts, deduplication, once-only stages, bounded immutable trace\n";
}

#include "src/system/common/recovery_timing.h"
#include "src/system/common/state_machine.h"
#include <cassert>
#include <iostream>

using lampda::common::RecoveryTiming;

static uint32_t testTime_ms = 0;
namespace lampda::hal {
extern "C" uint32_t time_ms() { return testTime_ms; }
}

int main()
{
  assert(RecoveryTiming::startup_timeout_ms == 3000);
  assert(RecoveryTiming::probe_timeout_ms(false) == 900);
  assert(RecoveryTiming::probe_timeout_ms(true) == 2400);
  assert(!RecoveryTiming::probe_timed_out(900, false));
  assert(RecoveryTiming::probe_timed_out(901, false));
  // The observed failure at 1047 ms should no longer reject an active recovery.
  assert(RecoveryTiming::probe_timed_out(1047, false));
  assert(!RecoveryTiming::probe_timed_out(1047, true));
  assert(!RecoveryTiming::probe_timed_out(2399, true));
  assert(!RecoveryTiming::probe_timed_out(2400, true));
  assert(RecoveryTiming::probe_timed_out(2401, true));
  assert(RecoveryTiming::probe_timed_out(3000, true));
  assert(RecoveryTiming::balancer_settle_ms == 250);
  assert(RecoveryTiming::recovery_ready_ms + RecoveryTiming::balancer_settle_ms <
         RecoveryTiming::startup_timeout_ms);
  enum class State { Startup, Error };
  lampda::common::StateMachine<State> machine(State::Startup, RecoveryTiming::startup_timeout_ms, State::Error);
  for (const uint32_t time : {1047u, 2400u, 2999u})
  {
    testTime_ms = time;
    machine.run();
    assert(machine.get_state() == State::Startup);
  }
  testTime_ms = 3000;
  machine.run();
  assert(machine.get_state() == State::Error && machine.state_changed_with_timeout());
  std::cout << "PASS: probe boundaries, observed failure, settling and actual state-machine timeout at 3000 ms\n";
}

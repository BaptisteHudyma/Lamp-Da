#pragma once

#include <cstdint>

namespace lampda::common {

struct RecoveryTiming
{
  static constexpr uint32_t startup_timeout_ms = 3000;
  static constexpr uint32_t initial_probe_timeout_ms = startup_timeout_ms * 3 / 10;
  static constexpr uint32_t recovery_ready_ms = startup_timeout_ms * 8 / 10;
  static constexpr uint32_t balancer_settle_ms = 250;

  static constexpr uint32_t probe_timeout_ms(bool recovery)
  {
    return recovery ? recovery_ready_ms : initial_probe_timeout_ms;
  }

  static constexpr bool probe_timed_out(uint32_t elapsed_ms, bool recovery)
  {
    return elapsed_ms > probe_timeout_ms(recovery);
  }
};

static_assert(RecoveryTiming::initial_probe_timeout_ms < RecoveryTiming::recovery_ready_ms,
              "Recovery requires a longer probe window than initial detection");
static_assert(RecoveryTiming::recovery_ready_ms + RecoveryTiming::balancer_settle_ms <
                      RecoveryTiming::startup_timeout_ms,
              "Recovery and balancer settling must fit within the existing startup timeout");

} // namespace lampda::common

#pragma once

#include <array>
#include <atomic>
#include <cstdint>

namespace lampda::common {

enum class RecoveryStage : uint8_t
{
  ChargerProbe,
  BalancerProbe,
  VbusGateRequested,
  StartupRejected,
  ChargerSetup,
  ChargeRequested,
  BalancerSetupFailed,
  Count
};

struct RecoveryEvent
{
  uint32_t time_ms = 0;
  RecoveryStage stage = RecoveryStage::ChargerProbe;
  int result = 0;
};

// One startup writer, CLI readers. Published entries are immutable, with no wraparound.
class RecoveryTrace
{
public:
  static constexpr uint32_t capacity = 16;

  void probe(RecoveryStage stage, int result, uint32_t time_ms)
  {
    const bool charger = stage == RecoveryStage::ChargerProbe;
    auto& attempts = charger ? charger_attempts : balancer_attempts;
    auto& last = charger ? charger_result : balancer_result;
    attempts.fetch_add(1, std::memory_order_relaxed);
    if (last.exchange(static_cast<uint32_t>(result), std::memory_order_relaxed) != static_cast<uint32_t>(result))
      append({time_ms, stage, result});
  }

  void once(RecoveryStage stage, int result, uint32_t time_ms)
  {
    const uint32_t bit = 1u << static_cast<uint8_t>(stage);
    if (seen & bit)
      return;
    seen |= bit;
    append({time_ms, stage, result});
  }

  uint32_t size() const { return published.load(std::memory_order_acquire); }
  const RecoveryEvent& event(uint32_t index) const { return events[index]; }
  uint32_t omitted() const { return dropped.load(std::memory_order_relaxed); }
  uint32_t charger_probes() const { return charger_attempts.load(std::memory_order_relaxed); }
  uint32_t balancer_probes() const { return balancer_attempts.load(std::memory_order_relaxed); }

private:
  void append(RecoveryEvent event)
  {
    const uint32_t index = published.load(std::memory_order_relaxed);
    if (index >= capacity)
    {
      dropped.fetch_add(1, std::memory_order_relaxed);
      return;
    }
    events[index] = event;
    published.store(index + 1, std::memory_order_release);
  }

  std::array<RecoveryEvent, capacity> events {};
  std::atomic<uint32_t> published {0}, dropped {0};
  std::atomic<uint32_t> charger_attempts {0}, balancer_attempts {0};
  std::atomic<uint32_t> charger_result {UINT32_MAX}, balancer_result {UINT32_MAX};
  uint32_t seen = 0;
};

static_assert(ATOMIC_INT_LOCK_FREE == 2, "Startup diagnostics require lock-free integer atomics");

} // namespace lampda::common

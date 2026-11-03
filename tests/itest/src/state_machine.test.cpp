#include <gtest/gtest.h>

#include "src/system/hal/time.h"

#include "src/system/common/state_machine.h"

// access simulation states
#include "simulator/include/simulator_state.h"
#include "simulator/include/hardware_influencer.h"

namespace lampda::common {

TEST(state_machine_test, state_timeout)
{
  enum class State
  {
    Startup,
    Error
  };

  ::simulator::time_mocks::reset();

  StateMachine<State> machine(State::Startup, 3000, State::Error);

  for (const uint32_t time: {1047u, 1353u, 550u})
  {
    hal::delay_ms(time);
    machine.run();
    ASSERT_EQ(machine.get_state(), State::Startup);
  }
  // overflow to error state
  hal::delay_ms(100);
  machine.run();

  ASSERT_EQ(machine.get_state(), State::Error);
  ASSERT_TRUE(machine.state_changed_with_timeout());
}

} // namespace lampda::common

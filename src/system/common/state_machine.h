/*! \file state_machine.h
    \brief Define a generic state machine logic.
*/

#pragma once

#include "src/system/hal/time.h"
#include <cstdint>

namespace lampda {
namespace common {

/**
 * \brief Define a state machine
 */
template<typename State> class StateMachine
{
public:
  /// Construct a state machine from the first given state
  StateMachine(const State s) : current(s), lastState(s), stateSetTime(hal::time_ms()), afterTimeoutState(s) {}
  /**
   * \brief Construct a state machine from the first given state, with a timeout to another
   * \param[in] s The initial state
   * \param[in] timeout The timeout after which we switch to the fallback state
   * \param[in] stateOnTimeout Falback state, reached on timeout
   */
  StateMachine(const State s, const uint32_t timeout, const State stateOnTimeout) :
    current(s),
    lastState(s),
    stateSetTime(hal::time_ms()),
    afterTimeoutState(s)
  {
    set_state(s, timeout, stateOnTimeout);
  }

  StateMachine(const StateMachine&) = delete;
  StateMachine& operator=(StateMachine const&) = delete;

  /**
   * \brief timeout check, and such, call it often
   */
  void run()
  {
    // there is a timeout set for this state (elapsed time comparison is safe across a time_ms wrap)
    if (isTimeoutSet and (hal::time_ms() - timeoutStart_ms) >= timeoutDuration_ms)
    {
      // timeout reached
      set_state(afterTimeoutState);
      changedWithTimeout = true;
    }
  }

  /**
   * \brief Set the machine state.
   * \param[in] s The new state to set
   * \param[in] forceUpdate Force the state update, even if the state is already active
   * \return True if the state was updated
   */
  bool set_state(const State s, bool forceUpdate = false)
  {
    if (not forceUpdate and s == current)
    {
      // ignore a set state with same value
      return false;
    }
    // update the state
    lastState = current;
    current = s;
    // reset the timeout time
    isTimeoutSet = false;
    timeoutStart_ms = 0;
    timeoutDuration_ms = 0;
    stateSetTime = hal::time_ms();

    changedWithTimeout = false;
    didStateJustChanged = true;
    return true;
  }

  /**
   * \brief set the new current state, with a timeout
   * \param[in] s the new state
   * \param[in] timeout The time out delay after which the state will switch automatically to stateOnTimeout
   * \param[in] stateOnTimeout State to reach after the timeout
   * \return true is the state changed
   */
  bool set_state(const State s, const uint32_t timeout, const State stateOnTimeout)
  {
    // set the new state if
    // - no timeout is active
    // - new state changes
    // - timeout state changes
    if (not isTimeoutSet or (s != current or afterTimeoutState != stateOnTimeout))
    {
      // force update state
      set_state(s, true);
      // set a timeout
      isTimeoutSet = true;
      // set after timeout state & timeout
      update_timeout(timeout);
      afterTimeoutState = stateOnTimeout;
      return true;
    }
    return false;
  }

  /**
   * \brief Update the current state timeout if a timeout is active
   */
  void update_timeout(const uint32_t timeout)
  {
    if (isTimeoutSet)
    {
      timeoutStart_ms = hal::time_ms();
      timeoutDuration_ms = timeout;
    }
  }

  /// Return true if the current state just changed. Resetted on read
  bool state_just_changed()
  {
    const bool temp = didStateJustChanged;
    didStateJustChanged = false;
    return temp;
  }

  /// return true if this state was reached with a timeout
  bool state_changed_with_timeout() { return changedWithTimeout; }

  /// Return the actual state
  State get_state() const { return current; }

  /// Return the state before this one
  State get_last_state() const { return lastState; }

  /// Return the state after the timeout. May be undefined
  State after_timeout_state() const { return afterTimeoutState; }

  /// use this in a state with a timeout set to skip to the next state directly
  void skip_timeout()
  {
    if (isTimeoutSet)
    {
      set_state(afterTimeoutState);
    }
  }

  /// Return the time at which this state was raised
  uint32_t get_state_raised_time() const { return stateSetTime; }

  /// Restart the time returned by \ref get_state_raised_time, without changing the state or its timeout
  void reset_state_raised_time() { stateSetTime = hal::time_ms(); }

private:
  /// The current state the algorithm is in
  State current;
  /// keep a reference to the last state
  State lastState;

  // The time this state was raised
  uint32_t stateSetTime;
  /// True if a timeout is active for this state
  bool isTimeoutSet = false;
  /// the time the timeout started, see \ref timeoutDuration_ms
  uint32_t timeoutStart_ms = 0;
  /// the delay after which this state times out and goes to \ref afterTimeoutState
  uint32_t timeoutDuration_ms = 0;
  // The state to go after a timout
  State afterTimeoutState;

  /// indicates that the state just changed, reseted on read
  bool didStateJustChanged = false;
  // indicates that this state was reched with a timeout
  bool changedWithTimeout = false;
};

} // namespace common
} // namespace lampda

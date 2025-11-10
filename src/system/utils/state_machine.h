#pragma once

/**
 * Define a generic state machina logic
 */

#include "src/system/platform/time.h"
#include <cstdint>

template<typename State> class StateMachine
{
public:
  StateMachine(const State s) : current(s), lastState(s), timeout_ms(0), afterTimeoutState(s) {}

  /**
   * \brief timeout check, and such, call it often
   */
  void run()
  {
    // there is a timeout set for this state
    if (isTimeoutSet and time_ms() >= timeout_ms)
    {
      // timeout reached
      set_state(afterTimeoutState);
    }
  }

  /**
   * \brief Set the machine state.
   * \param[in] s The new state to set
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
    timeout_ms = 0;
    stateSetTime = time_ms();

    didStateJustChanged = true;
    return true;
  }

  /**
   * \brief set the new current state, with a timeout
   * \param[in] s the new state
   * \param[in] timeout The time out delay after which the state will switch automatically to \ref stateOnTimeout
   * \param[in] stateOnTimeout
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
      // potential clock overflow, be careful
      timeout_ms = time_ms() + timeout;
    }
  }

  bool state_just_changed()
  {
    const bool temp = didStateJustChanged;
    didStateJustChanged = false;
    return temp;
  }

  /// Return the actual state
  State get_state() const { return current; }

  /// Return the state before this one
  State get_last_state() const { return lastState; }

  /// use this in a state with a timeout set to skip to the next state directly
  void skip_timeout()
  {
    if (isTimeoutSet)
    {
      set_state(afterTimeoutState);
    }
  }

  // return the time at which this state was raised
  uint32_t get_state_raised_time() const { return stateSetTime; }

private:
  /// The current state the algorithm is in
  State current;
  /// keep a reference to the last state
  State lastState;

  // The time this state was raised
  uint32_t stateSetTime;
  // prevent some errors with clock value wrap
  bool isTimeoutSet;
  /// the time this state should timeout and go to /ref afterTimeoutState
  uint32_t timeout_ms;
  // The state to go after a timout
  State afterTimeoutState;

  /// indicates that the state just changed, reseted on read
  bool didStateJustChanged;
};

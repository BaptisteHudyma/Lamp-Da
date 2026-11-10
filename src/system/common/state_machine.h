/*! \file state_machine.h
    \brief Define a generic state machine logic.
*/

#pragma once

#include "src/system/hal/mutex.h"
#include "src/system/hal/time.h"
#include <cstdint>

namespace lampda {
namespace common {

/**
 * \brief Define a state machine.
 * Thread safe: all the methods are synchronized, a state change and its timeout are always seen together.
 * Do not call from an interrupt.
 */
template<typename State> class StateMachine
{
  /// Hold the machine mutex for the scope
  struct Lock
  {
    explicit Lock(hal::mutex::hal_mutex_t& mutex) : _mutex(mutex) { hal::mutex::hal_mutex_lock(&_mutex); }
    ~Lock() { hal::mutex::hal_mutex_unlock(&_mutex); }
    Lock(const Lock&) = delete;
    Lock& operator=(const Lock&) = delete;

  private:
    hal::mutex::hal_mutex_t& _mutex;
  };

public:
  /// Construct a state machine from the first given state
  StateMachine(const State s) : current(s), lastState(s), stateSetTime(hal::time_ms()), afterTimeoutState(s)
  {
    hal::mutex::hal_mutex_init(&mutex);
  }
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
    hal::mutex::hal_mutex_init(&mutex);
    Lock lock(mutex);
    set_state_with_timeout_locked(s, timeout, stateOnTimeout);
  }

  StateMachine(const StateMachine&) = delete;
  StateMachine& operator=(StateMachine const&) = delete;

  /**
   * \brief timeout check, and such, call it often
   */
  void run()
  {
    Lock lock(mutex);
    // there is a timeout set for this state (elapsed time comparison is safe across a time_ms wrap)
    if (isTimeoutSet and (hal::time_ms() - timeoutStart_ms) >= timeoutDuration_ms)
    {
      // timeout reached
      set_state_locked(afterTimeoutState, false);
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
    Lock lock(mutex);
    return set_state_locked(s, forceUpdate);
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
    Lock lock(mutex);
    return set_state_with_timeout_locked(s, timeout, stateOnTimeout);
  }

  /**
   * \brief Same as set_state(s, timeout, stateOnTimeout), unless the machine is in \p blockingState.
   * The check and the update are atomic: another thread can not switch to \p blockingState in between.
   * \return false if the machine is in \p blockingState, true otherwise (even if the state did not change)
   */
  bool set_state_unless(const State blockingState, const State s, const uint32_t timeout, const State stateOnTimeout)
  {
    Lock lock(mutex);
    if (current == blockingState)
      return false;
    set_state_with_timeout_locked(s, timeout, stateOnTimeout);
    return true;
  }

  /**
   * \brief Update the current state timeout if a timeout is active
   */
  void update_timeout(const uint32_t timeout)
  {
    Lock lock(mutex);
    update_timeout_locked(timeout);
  }

  /// Return true if the current state just changed. Resetted on read
  bool state_just_changed()
  {
    Lock lock(mutex);
    const bool temp = didStateJustChanged;
    didStateJustChanged = false;
    return temp;
  }

  /// return true if this state was reached with a timeout
  bool state_changed_with_timeout() const
  {
    Lock lock(mutex);
    return changedWithTimeout;
  }

  /// Return the actual state
  State get_state() const
  {
    Lock lock(mutex);
    return current;
  }

  /// Return the state before this one
  State get_last_state() const
  {
    Lock lock(mutex);
    return lastState;
  }

  /// Return the state after the timeout. May be undefined
  State after_timeout_state() const
  {
    Lock lock(mutex);
    return afterTimeoutState;
  }

  /**
   * \brief Return a value changed on every state set (timeout target included), to detect a concurrent update.
   * See skip_timeout(const uint32_t)
   */
  uint32_t get_state_generation() const
  {
    Lock lock(mutex);
    return stateGeneration;
  }

  /// use this in a state with a timeout set to skip to the next state directly
  void skip_timeout()
  {
    Lock lock(mutex);
    if (isTimeoutSet)
    {
      set_state_locked(afterTimeoutState, false);
    }
  }

  /**
   * \brief Skip the timeout, only if the state was not set since \p generation was read (see get_state_generation).
   * A skip decided on a state that another thread replaced meanwhile is dropped.
   * \return true if the timeout was skipped
   */
  bool skip_timeout(const uint32_t generation)
  {
    Lock lock(mutex);
    if (not isTimeoutSet or generation != stateGeneration)
      return false;
    set_state_locked(afterTimeoutState, false);
    return true;
  }

  /// Return the time at which this state was raised
  uint32_t get_state_raised_time() const
  {
    Lock lock(mutex);
    return stateSetTime;
  }

  /// Restart the time returned by \ref get_state_raised_time, without changing the state or its timeout
  void reset_state_raised_time()
  {
    Lock lock(mutex);
    stateSetTime = hal::time_ms();
  }

private:
  /// set_state(s, forceUpdate), call with the mutex held
  bool set_state_locked(const State s, const bool forceUpdate)
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
    ++stateGeneration;

    changedWithTimeout = false;
    didStateJustChanged = true;
    return true;
  }

  /// set_state(s, timeout, stateOnTimeout), call with the mutex held
  bool set_state_with_timeout_locked(const State s, const uint32_t timeout, const State stateOnTimeout)
  {
    // set the new state if
    // - no timeout is active
    // - new state changes
    // - timeout state changes
    if (not isTimeoutSet or (s != current or afterTimeoutState != stateOnTimeout))
    {
      // force update state
      set_state_locked(s, true);
      // timeout target and delay first, the timeout is active once all are set
      afterTimeoutState = stateOnTimeout;
      timeoutStart_ms = hal::time_ms();
      timeoutDuration_ms = timeout;
      isTimeoutSet = true;
      return true;
    }
    return false;
  }

  /// update_timeout(timeout), call with the mutex held
  void update_timeout_locked(const uint32_t timeout)
  {
    if (isTimeoutSet)
    {
      timeoutStart_ms = hal::time_ms();
      timeoutDuration_ms = timeout;
    }
  }

  /// Guards all the members below. Not recursive
  mutable hal::mutex::hal_mutex_t mutex;

  /// The current state the algorithm is in
  State current;
  /// keep a reference to the last state
  State lastState;

  // The time this state was raised
  uint32_t stateSetTime;
  /// Changed on every state set, see get_state_generation()
  uint32_t stateGeneration = 0;
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

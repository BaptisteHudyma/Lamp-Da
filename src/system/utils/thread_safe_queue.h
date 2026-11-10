/*! \file thread_safe_queue.h
    \brief Fixed capacity queue, safe to share between threads.
*/

#ifndef UTILS_THREAD_SAFE_QUEUE_H
#define UTILS_THREAD_SAFE_QUEUE_H

#include <cstddef>

#include <etl/optional.h>
#include <etl/queue.h>

#include "src/system/hal/mutex.h"

namespace lampda {
namespace utils {

/**
 * \brief Fixed capacity FIFO queue, guarded by a mutex.
 * Push and pop can be called from any thread, but not from an interrupt.
 * The lock is only held while copying an item, never while the caller handles it.
 */
template<typename T, size_t N> class ThreadSafeQueue
{
public:
  ThreadSafeQueue() { hal::mutex::hal_mutex_init(&_mutex); }

  ThreadSafeQueue(const ThreadSafeQueue&) = delete;
  ThreadSafeQueue& operator=(const ThreadSafeQueue&) = delete;

  /**
   * \brief Push an item at the end of the queue
   * \return false if the queue is full and the item was dropped
   */
  bool push(const T& item)
  {
    Lock lock(_mutex);
    // pushing in a full etl::queue is an assert
    if (_queue.full())
      return false;
    _queue.push(item);
    return true;
  }

  /**
   * \brief Pop the oldest item
   * \return The popped item, or nothing if the queue is empty
   */
  etl::optional<T> pop()
  {
    Lock lock(_mutex);
    if (_queue.empty())
      return etl::nullopt;
    etl::optional<T> item(_queue.front());
    _queue.pop();
    return item;
  }

  /// Return the number of stored items
  size_t size()
  {
    Lock lock(_mutex);
    return _queue.size();
  }

  /// Drop all stored items
  void clear()
  {
    Lock lock(_mutex);
    _queue.clear();
  }

  /// Maximum number of stored items
  static constexpr size_t capacity() { return N; }

private:
  /// Hold the mutex for the scope
  struct Lock
  {
    explicit Lock(hal::mutex::hal_mutex_t& m) : _m(m) { hal::mutex::hal_mutex_lock(&_m); }
    ~Lock() { hal::mutex::hal_mutex_unlock(&_m); }
    Lock(const Lock&) = delete;
    Lock& operator=(const Lock&) = delete;

  private:
    hal::mutex::hal_mutex_t& _m;
  };

  etl::queue<T, N> _queue;
  hal::mutex::hal_mutex_t _mutex;
};

} // namespace utils
} // namespace lampda

#endif

/*! \file queue.h
    \brief Define a queue object with constant memory footprint
*/

#ifndef UTILS_QUEUE
#define UTILS_QUEUE

#include <array>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <optional>

namespace lampda {
namespace utils {

template<typename T, size_t MaxElements = 10> struct Queue
{
  static_assert(MaxElements > 1, "Queue: cannot define a one or less elements queue");

  struct Element
  {
    uint32_t raisedTime;
    T _data;
  };

  bool has_elements() const { return currentStoredEvents > 0; }

  /**
   * \brief Add an element to the queue.
   * \return false if the element cannot be added
   */
  bool enqueue_element(const T& element)
  {
    if (currentStoredEvents >= MaxElements)
      return false;
    std::scoped_lock lock(mut);

    Element e;
    e._data = element;
    _queueData[currentStoredEvents] = e;

    currentStoredEvents++;
    return true;
  }

  /**
   * \brief Remove and return the first element from the queue, or nothing
   */
  std::optional<T> dequeue()
  {
    if (currentStoredEvents <= 0)
      return std::nullopt;

    std::scoped_lock lock(mut);

    // store first element
    const Element toReturn = _queueData[0];

    // move all other elements toward the front
    for (size_t i = 1; i < currentStoredEvents; i++)
    {
      _queueData[i - 1] = _queueData[i];
    }
    // remove the element
    currentStoredEvents -= 1;
    return toReturn._data;
  }

private:
  std::mutex mut;
  size_t currentStoredEvents = 0;
  std::array<Element, MaxElements> _queueData;
};

} // namespace utils
} // namespace lampda

#endif

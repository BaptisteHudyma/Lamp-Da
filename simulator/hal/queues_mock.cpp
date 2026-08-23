/*! \file queues_mock.cpp
    \brief Mock of the board queues
*/

#include "src/system/hal/queues.h"
#include "src/system/hal/time.h"

#include <queue>
#include <vector>
#include <cstring>
#include <mutex>

#define HAL_QUEUES_CPP

namespace simulator {
}

namespace lampda {
namespace hal {
namespace queues {

struct QueueWrapp
{
  QueueWrapp(size_t itemSize, uint32_t queueLength) : uxLength(queueLength), uxItemSize(itemSize) {}

  std::queue<std::vector<uint8_t>> queue;

  size_t uxLength; /*< The length of the queue defined as the number of items it will hold, not the number of bytes. */
  size_t uxItemSize; /*< The size of each items that the queue will hold. */

  mutable std::mutex mtx;
};

QueueHandle_t HAL_create_queue(size_t itemSize, uint32_t queueLength)
{
  // create a new queue
  return (QueueHandle_t)(new QueueWrapp(itemSize, queueLength));
}

void HAL_delete_queue(QueueHandle_t handle)
{
  if (handle)
    delete static_cast<QueueWrapp*>(handle);
}

HAL_queue_status_t HAL_queue_send(QueueHandle_t handle, const void* item, uint32_t timeoutMs)
{
  if (!handle || !item)
    return HAL_queue_status_t::HAL_QUEUE_ERR_INVALID_PARAM;
  //
  auto* q = static_cast<QueueWrapp*>(handle);

  const auto& timeoutTime = hal::time_ms() + timeoutMs;
  while (q->queue.size() >= q->uxLength)
  {
    if (timeoutMs != UINT32_MAX && hal::time_ms() >= timeoutTime)
    {
      return HAL_QUEUE_ERR_TIMEOUT;
    }

    hal::delay_ms(1);
  }
  if (q->queue.size() >= q->uxLength)
    return HAL_queue_status_t::HAL_QUEUE_ERR_FULL;

  std::lock_guard<std::mutex> lock(q->mtx);

  std::vector<uint8_t> data(static_cast<const uint8_t*>(item), static_cast<const uint8_t*>(item) + q->uxItemSize);
  q->queue.push(std::move(data));

  return HAL_queue_status_t::HAL_QUEUE_OK;
}

HAL_queue_status_t HAL_queue_receive(QueueHandle_t handle, void* const outItem, uint32_t timeoutMs)
{
  if (!handle || !outItem)
    return HAL_queue_status_t::HAL_QUEUE_ERR_INVALID_PARAM;

  auto* q = static_cast<QueueWrapp*>(handle);

  const auto& timeoutTime = hal::time_ms() + timeoutMs;
  while (q->queue.empty())
  {
    if (timeoutMs != UINT32_MAX && hal::time_ms() >= timeoutTime)
    {
      return HAL_QUEUE_ERR_TIMEOUT;
    }

    hal::delay_ms(1);
  }

  if (q->queue.empty())
    return HAL_queue_status_t::HAL_QUEUE_ERR_EMPTY;

  std::lock_guard<std::mutex> lock(q->mtx);

  const auto& data = q->queue.front();
  memcpy(outItem, data.data(), data.size());
  q->queue.pop();

  return HAL_queue_status_t::HAL_QUEUE_OK;
}

} // namespace queues
} // namespace hal
} // namespace lampda

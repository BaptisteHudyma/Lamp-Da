#ifndef HAL_QUEUES_CPP
#define HAL_QUEUES_CPP

#include "queues.h"

#include <Arduino.h>

namespace lampda {
namespace hal {
namespace queues {

QueueHandle_t HAL_create_queue(size_t itemSize, uint32_t queueLength)
{
  //
  return xQueueCreate(queueLength, itemSize);
}

void HAL_delete_queue(QueueHandle_t handle)
{
  //
}

HAL_queue_status_t HAL_queue_send(QueueHandle_t handle, const void* item, uint32_t timeoutMs)
{
  if (!handle || !item)
    return HAL_queue_status_t::HAL_QUEUE_ERR_INVALID_PARAM;

  const auto res = xQueueSend(handle, item, ms2tick(timeoutMs));
  switch (res)
  {
    case pdPASS:
      return HAL_queue_status_t::HAL_QUEUE_OK;
    case errQUEUE_FULL:
      return HAL_queue_status_t::HAL_QUEUE_ERR_FULL;
  }
  return HAL_queue_status_t::HAL_QUEUE_ERR_OTHER;
}

HAL_queue_status_t HAL_queue_receive(QueueHandle_t handle, void* const outItem, uint32_t timeoutMs)
{
  if (!handle || !outItem)
    return HAL_queue_status_t::HAL_QUEUE_ERR_INVALID_PARAM;

  const auto res = xQueueReceive(handle, outItem, ms2tick(timeoutMs));
  switch (res)
  {
    case pdPASS:
      return HAL_queue_status_t::HAL_QUEUE_OK;
    case errQUEUE_EMPTY:
      return HAL_queue_status_t::HAL_QUEUE_ERR_EMPTY;
  }
  return HAL_queue_status_t::HAL_QUEUE_ERR_OTHER;
}

} // namespace queues
} // namespace hal
} // namespace lampda

#endif

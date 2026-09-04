/*! \file hal/queues.h
    \brief Interface for platform-specific queue operations (e.g., FreeRTOS xQueue).
*/

// do not use pragma once here, has this can be mocked
#ifndef HAL_QUEUES_H
#define HAL_QUEUES_H

#ifdef __cplusplus

#include <cstddef>
#include <cstdint>

namespace lampda {
namespace hal {
/// Implementation of inter thread queues
namespace queues {

extern "C" {
#endif

  typedef void* QueueHandle_t;

  /// Define the max number of queue objets that can be defined
  static constexpr uint32_t MaxStaticQueues = 5;

  /**
   * \brief Queue operation status codes
   */
  typedef enum
  {
    HAL_QUEUE_OK = 0,
    HAL_QUEUE_ERR_INVALID_PARAM = -1,
    HAL_QUEUE_ERR_TIMEOUT = -2,
    HAL_QUEUE_ERR_FULL = -3,
    HAL_QUEUE_ERR_EMPTY = -4,
    HAL_QUEUE_ERR_OTHER = -5
  } HAL_queue_status_t;

  /**
   * \brief Create a queue with the specified item size and maximum length.
   * \param[in] itemSize Size of each item in bytes (abstracts the data type)
   * \param[in] queueLength Maximum number of items the queue can hold
   * \param[in] staticIndex Index of the static object to use. Limited to MaxStaticQueues. THEY CANNOT BE SHARED BETWEEN
   * QUEUES
   * \param[in, out] buffer Preallocated static buffer to itemSize * queueLength
   * \return The allocated queue handle
   */
  QueueHandle_t HAL_create_queue(size_t itemSize, uint32_t queueLength, uint32_t staticIndex, uint8_t* buffer);

  /**
   * \brief Delete/free a queue and release its underlying resources.
   * \param[in] handle Handle of the queue to delete
   */
  void HAL_delete_queue(QueueHandle_t handle);

  /**
   * \brief Send an item to the queue.
   * \param[in] handle Handle of the queue
   * \param[in] item Pointer to the item to send
   * \param[in] timeoutMs Timeout in milliseconds. 0 = non-blocking, UINT32_MAX = block indefinitely
   * \return HAL_QUEUE_OK on success, HAL_QUEUE_ERR_FULL if queue is full, HAL_QUEUE_ERR_TIMEOUT on timeout
   */
  HAL_queue_status_t HAL_queue_send(QueueHandle_t handle, const void* item, uint32_t timeoutMs);

  /**
   * \brief Receive an item from the queue. Blocks until an item is available or timeout expires.
   * \param[in] handle Handle of the queue
   * \param[out] outItem Pointer to buffer to store the received item
   * \param[in] timeoutMs Timeout in milliseconds. 0 = non-blocking
   * \return HAL_QUEUE_OK on success, HAL_QUEUE_ERR_EMPTY if queue is empty, HAL_QUEUE_ERR_TIMEOUT on timeout
   */
  HAL_queue_status_t HAL_queue_receive(QueueHandle_t handle, void* const outItem, uint32_t timeoutMs);

  /**
   * \brief Return the number of items currently in a queue
   */
  size_t HAL_queue_get_number_of_items(QueueHandle_t handle);

#ifdef __cplusplus
}
} // namespace queues
} // namespace hal
} // namespace lampda
#endif

#endif

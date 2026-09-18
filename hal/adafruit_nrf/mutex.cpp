#include "src/system/hal/mutex.h"

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "wiring.h"
#include <cstring>

namespace lampda {
namespace hal {
/// Define mutex specific hal.
namespace mutex {

typedef struct
{
  StaticSemaphore_t static_mutex;
  SemaphoreHandle_t handle;
} freertos_mutex_storage_t;

_Static_assert(sizeof(freertos_mutex_storage_t) <= HAL_MUTEX_OPAQUE_SIZE,
               "hal_mutex.h: HAL_MUTEX_OPAQUE_SIZE too small for FreeRTOS static mutex storage");
_Static_assert(HAL_MUTEX_OPAQUE_ALIGN % __alignof__(freertos_mutex_storage_t) == 0,
               "hal_mutex.h: HAL_MUTEX_OPAQUE_ALIGN incompatible with FreeRTOS static mutex storage alignment");

static inline freertos_mutex_storage_t* native(hal_mutex_t* mutex)
{
  return (freertos_mutex_storage_t*)(void*)mutex->opaque;
}

static constexpr size_t MaxStaticMutex = 32;
static StaticSemaphore_t staticSemaphoreArray[MaxStaticMutex];
inline static size_t currentStaticTaskIndex = 0;

int hal_mutex_init(hal_mutex_t* mutex)
{
  freertos_mutex_storage_t* store;

  if (mutex == NULL)
    return HAL_MUTEX_ERROR;

  memset(mutex->opaque, 0, sizeof(mutex->opaque));
  store = native(mutex);

  store->handle = xSemaphoreCreateCountingStatic(1, 1, &store->static_mutex);
  if (store->handle == NULL)
    return HAL_MUTEX_ERROR;

  mutex->is_initialized = 1U;
  return HAL_MUTEX_OK;
}

bool true_mutex_lock(hal_mutex_t* mutex, int maxDelay)
{
  if (mutex == NULL || !mutex->is_initialized)
    return false;

  freertos_mutex_storage_t* store;
  store = native(mutex);

  if (isInISR())
  {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    return xSemaphoreTakeFromISR(store->handle, &xHigherPriorityTaskWoken) == pdTRUE;
  }
  else
    return xSemaphoreTake(store->handle, maxDelay) == pdTRUE;
}

bool true_mutex_unlock(hal_mutex_t* mutex)
{
  if (mutex == NULL || !mutex->is_initialized)
    return false;

  freertos_mutex_storage_t* store;
  store = native(mutex);

  if (isInISR())
  {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    const auto res = xSemaphoreGiveFromISR(store->handle, &xHigherPriorityTaskWoken);
    return res == pdTRUE;
  }
  else
    return xSemaphoreGive(store->handle) == pdTRUE;
}

void hal_mutex_lock(hal_mutex_t* m) { true_mutex_lock(m, portMAX_DELAY); }

void hal_mutex_unlock(hal_mutex_t* m) { true_mutex_unlock(m); }

/* Returns 0 on success, -1 if already locked. */
int hal_mutex_trylock(hal_mutex_t* m) { return true_mutex_lock(m, 0) ? 0 : -1; }

} // namespace mutex
} // namespace hal
} // namespace lampda

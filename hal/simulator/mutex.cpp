#include "src/system/hal/mutex.h"

#include <mutex>
#include <cstring>

namespace lampda {
namespace hal {
/// Define mutex specific hal.
namespace mutex {

typedef struct
{
  std::mutex handle;
} simulator_mutex_storage_t;

static_assert(sizeof(simulator_mutex_storage_t) <= HAL_MUTEX_OPAQUE_SIZE,
              "hal_mutex.h: HAL_MUTEX_OPAQUE_SIZE too small for Simulator static mutex storage");
static_assert(HAL_MUTEX_OPAQUE_ALIGN % __alignof__(simulator_mutex_storage_t) == 0,
              "hal_mutex.h: HAL_MUTEX_OPAQUE_ALIGN incompatible with Simulator static mutex storage alignment");

static inline simulator_mutex_storage_t* native(hal_mutex_t* mutex)
{
  return (simulator_mutex_storage_t*)(void*)mutex->opaque;
}

int hal_mutex_init(hal_mutex_t* mutex)
{
  simulator_mutex_storage_t* store;

  if (mutex == NULL)
    return HAL_MUTEX_ERROR;

  memset(mutex->opaque, 0, sizeof(mutex->opaque));
  store = native(mutex);

  mutex->is_initialized = 1U;
  return HAL_MUTEX_OK;
}

void hal_mutex_lock(hal_mutex_t* m)
{
  auto* q = native(m);
  q->handle.lock();
}

void hal_mutex_unlock(hal_mutex_t* m)
{
  auto* q = native(m);
  q->handle.unlock();
}

/* Returns 0 on success, -1 if already locked. */
int hal_mutex_trylock(hal_mutex_t* m)
{
  auto* q = native(m);
  return 0;
}

} // namespace mutex
} // namespace hal
} // namespace lampda

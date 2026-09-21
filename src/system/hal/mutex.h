/*
 * mutex.h - Portable mutex HAL.
 *
 * Goal: provide a single, statically-allocatable mutex abstraction so
 * driver code (e.g. FUSB302.c) does not need to carry OS-specific
 * #ifdefs to protect shared hardware resources (e.g. TCPC_REG_MEASURE).
 *
 * Usage:
 *   HAL_MUTEX_DEFINE(measure_lock);
 *
 *   void some_init(void)
 *   {
 *     hal_mutex_init(&measure_lock);
 *   }
 *
 *   void some_critical_section(void)
 *   {
 *     hal_mutex_lock(&measure_lock);
 *     ... touch shared register ...
 *     hal_mutex_unlock(&measure_lock);
 *   }
 */

#ifndef HAL_MUTEX_H
#define HAL_MUTEX_H

#ifdef __cplusplus

#include <cstdint>

namespace lampda {
namespace hal {
/// Define mutex specific hal.
namespace mutex {

extern "C" {

#endif

/* --- Status codes returned by all hal_mutex_* functions --- */
#define HAL_MUTEX_OK      (0)
#define HAL_MUTEX_ERROR   (-1)
#define HAL_MUTEX_TIMEOUT (-2)

#define HAL_MUTEX_OPAQUE_SIZE  128U
#define HAL_MUTEX_OPAQUE_ALIGN 8U

  typedef struct hal_mutex
  {
    /* Backend-owned storage. Never access directly from portable code. */
    uint8_t opaque[HAL_MUTEX_OPAQUE_SIZE] __attribute__((aligned(HAL_MUTEX_OPAQUE_ALIGN)));
    /* Set to 1 by hal_mutex_init(); used to catch use-before-init bugs. */
    uint8_t is_initialized;
  } hal_mutex_t;

  int hal_mutex_init(hal_mutex_t* m);

  void hal_mutex_lock(hal_mutex_t* m);

  void hal_mutex_unlock(hal_mutex_t* m);

  int hal_mutex_trylock(hal_mutex_t* m);

#ifdef __cplusplus
}
}
}
}
#endif

#endif /* HAL_MUTEX_H */

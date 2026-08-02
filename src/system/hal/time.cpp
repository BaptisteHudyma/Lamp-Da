#ifndef HAL_TIME_CPP
#define HAL_TIME_CPP

#include "time.h"

// Use the Arduino defined functions
#include "delay.h"

#ifdef __cplusplus

#include "src/system/utils/utils.h"
namespace lampda {
namespace hal {

extern "C" {
#endif

  uint32_t time_ms(void) { return millis(); }

  uint64_t time_us(void) { return micros(); }

  void delay_ms(uint32_t dwMs) { delay(dwMs); }

  void delay_us(uint64_t dwUs) { delayMicroseconds(dwUs); }

#ifdef __cplusplus
}
} // namespace hal
} // namespace lampda
#endif

#endif

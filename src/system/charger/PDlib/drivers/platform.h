
#include <Arduino.h>

void platform_usleep(uint64_t us) { delayMicroseconds(us); }

void delay_us(uint64_t us) { platform_usleep(us); }

void delay_ms(uint64_t ms) { delay(ms); }
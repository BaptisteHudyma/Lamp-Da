#ifndef PLATFORM_TIME_CPP
#define PLATFORM_TIME_CPP

#include "time.h"

// Use the Arduino defined functions
#include "delay.h"

uint32_t time_ms(void) { return millis(); }

uint32_t time_us(void) { return micros(); }

void delay_ms(uint32_t dwMs) { delay(dwMs); }

void delay_us(uint32_t dwUs) { delayMicroseconds(dwUs); }

#endif
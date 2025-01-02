#ifndef MOCK_TIME_H
#define MOCK_TIME_H

#define PLATFORM_TIME

#include <SFML/Graphics.hpp>

// time
static sf::Clock s_clock;

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
  inline uint32_t time_ms(void) { return s_clock.getElapsedTime().asMilliseconds(); }

  inline uint32_t time_us(void) { return s_clock.getElapsedTime().asMicroseconds(); }

  inline void delay_ms(uint32_t dwMs) { sf::sleep(sf::milliseconds(dwMs)); }

  inline void delay_us(uint32_t dwUs) { sf::sleep(sf::microseconds(dwUs)); }

#ifdef __cplusplus
}
#endif

#endif

#include "src/system/platform/time.h"

#define PLATFORM_TIME_CPP

#include <SFML/Graphics.hpp>

// time
static sf::Clock s_clock;

#include <stdint.h>

uint32_t time_ms(void) { return s_clock.getElapsedTime().asMilliseconds(); }

uint32_t time_us(void) { return s_clock.getElapsedTime().asMicroseconds(); }

void delay_ms(uint32_t dwMs) { sf::sleep(sf::milliseconds(dwMs)); }

void delay_us(uint32_t dwUs) { sf::sleep(sf::microseconds(dwUs)); }
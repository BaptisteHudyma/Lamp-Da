#ifndef FAKE_ARDUINO_H
#define FAKE_ARDUINO_H

//
// minimal Arduino.h replacement
//

#include <string>
#include <type_traits>

#include <cstdint>
#include <cmath>

// constants
using String = std::string;

//
// lampda-specific hacks
//

using byte = uint8_t;

template<typename T, typename U> T random(T min, U max)
{
  uint32_t n = random();
  // bad random, but who cares?
  return (n % (max - min)) + min;
}

template<typename T> T random(T max) { return random(0, max); }

//
// ambiguous min/fmin/max/fmax/abs
//

static constexpr double min(double a, double b) { return fmin(a, b); }

static constexpr double max(double a, double b) { return fmax(a, b); }

using std::abs;

//
// misc functions
//

float radians(float degrees) { return (degrees / 180.f) * M_PI; }

template<typename T, typename V, typename U> static constexpr T constrain(const T& a, const V& mini, const U& maxi)
{
  if (a <= mini)
    return mini;
  if (a >= maxi)
    return maxi;
  return a;
}

template<typename T, typename V> static constexpr T pow(const T& a, const V& b)
{
  T acc = 1;
  for (V i = 0; i < b; ++i)
  {
    acc *= a;
  }
  return acc;
}

//
// emulate time_ms()
//

static uint64_t globalMillis;

#endif

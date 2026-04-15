/*! \file input_output.h
    \brief Definition of the used pins of the RGB indicator.
*/

#pragma once

#include "src/system/platform/gpio.h"

namespace lampda {
namespace utils {

// RGB indicator details
constexpr platform::gpio::DigitalPin::GPIO RedIndicator = platform::gpio::DigitalPin::GPIO::gpio0;
constexpr platform::gpio::DigitalPin::GPIO GreenIndicator = platform::gpio::DigitalPin::GPIO::gpio1;
constexpr platform::gpio::DigitalPin::GPIO BlueIndicator = platform::gpio::DigitalPin::GPIO::gpio2;

} // namespace utils
} // namespace lampda

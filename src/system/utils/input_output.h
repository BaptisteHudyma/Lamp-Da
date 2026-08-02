/*! \file input_output.h
    \brief Definition of the used pins of the RGB indicator.
*/

#pragma once

#include "src/system/hal/gpio.h"

namespace lampda {
namespace utils {

// RGB indicator details
constexpr hal::gpio::DigitalPin::GPIO RedIndicator = hal::gpio::DigitalPin::GPIO::gpio0;
constexpr hal::gpio::DigitalPin::GPIO GreenIndicator = hal::gpio::DigitalPin::GPIO::gpio1;
constexpr hal::gpio::DigitalPin::GPIO BlueIndicator = hal::gpio::DigitalPin::GPIO::gpio2;

} // namespace utils
} // namespace lampda

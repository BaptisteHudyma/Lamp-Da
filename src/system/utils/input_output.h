/*! \file input_output.h
    \brief Definition of the used pins of the RGB indicator.
*/

#pragma once

#include "src/system/platform/gpio.h"

// RGB indicator details
constexpr DigitalPin::GPIO RedIndicator = DigitalPin::GPIO::gpio0;
constexpr DigitalPin::GPIO GreenIndicator = DigitalPin::GPIO::gpio1;
constexpr DigitalPin::GPIO BlueIndicator = DigitalPin::GPIO::gpio2;

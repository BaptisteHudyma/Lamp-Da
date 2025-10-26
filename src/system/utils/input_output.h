#pragma once

#include "src/system/platform/gpio.h"

// The button pullup pin (one button pin to GND, the other to this pin)
constexpr DigitalPin::GPIO buttonPin = DigitalPin::GPIO::gpio3;

// RGB indicator details
constexpr DigitalPin::GPIO RedIndicator = DigitalPin::GPIO::gpio0;
constexpr DigitalPin::GPIO GreenIndicator = DigitalPin::GPIO::gpio1;
constexpr DigitalPin::GPIO BlueIndicator = DigitalPin::GPIO::gpio2;

static const DigitalPin ButtonPin(buttonPin);

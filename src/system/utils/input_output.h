#pragma once

#include "src/system/platform/gpio.h"

// The button pin (one button pin to GND, the other to this pin)
constexpr DigitalPin::GPIO buttonPin = DigitalPin::GPIO::p6;

static DigitalPin ButtonPin(buttonPin);

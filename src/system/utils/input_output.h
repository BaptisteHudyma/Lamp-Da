#pragma once

#include "src/system/platform/gpio.h"

// The button pullup pin (one button pin to GND, the other to this pin)
constexpr DigitalPin::GPIO buttonPin = DigitalPin::GPIO::p6;

// RGB indicator details
constexpr DigitalPin::GPIO RedIndicator = DigitalPin::GPIO::p8;
constexpr DigitalPin::GPIO GreenIndicator = DigitalPin::GPIO::p4;
constexpr DigitalPin::GPIO BlueIndicator = DigitalPin::GPIO::p7;

static DigitalPin ButtonPin(buttonPin);

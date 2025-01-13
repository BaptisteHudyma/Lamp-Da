#pragma once

#include "src/system/platform/gpio.h"

// The button pin (one button pin to GND, the other to this pin)
static DigitalPin ButtonPin(DigitalPin::GPIO::p6);

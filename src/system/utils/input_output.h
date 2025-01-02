#pragma once

#include "src/system/platform/gpio.h"

// The button pin (one button pin to GND, the other to this pin)
static DigitalPin ButtonPin(DigitalPin::GPIO::p6);

// Pins for the led on the button
static DigitalPin ButtonRedPin(DigitalPin::GPIO::p8);
static DigitalPin ButtonGreenPin(DigitalPin::GPIO::p4);
static DigitalPin ButtonBluePin(DigitalPin::GPIO::p7);
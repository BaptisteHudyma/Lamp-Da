#ifndef MOCK_GPIO_H
#define MOCK_GPIO_H

#define PLATFORM_GPIO_CPP

#include "src/system/platform/gpio.h"

DigitalPin::DigitalPin(GPIO pin) {}
DigitalPin::~DigitalPin() = default;
DigitalPin::DigitalPin(const DigitalPin& other) = default;

DigitalPin& DigitalPin::operator=(const DigitalPin& other) = default;

void DigitalPin::set_pin_mode(Mode mode) {}

bool DigitalPin::is_high() const { return true; }

void DigitalPin::set_high(bool is_high) {}

void DigitalPin::write(uint16_t value) {}

uint16_t DigitalPin::read() const { return 0; }

int DigitalPin::pin() const { return 0; }

void DigitalPin::attach_callback(voidFuncPtr func, Interrupt mode) {}

#endif
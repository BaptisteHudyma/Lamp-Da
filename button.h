#ifndef BUTTON_H
#define BUTTON_H

#include <Adafruit_NeoPixel.h>
#include <functional>

#define BUTTON_PIN D2

void handle_button_events(std::function<void(uint8_t, uint32_t)> callback);

#endif

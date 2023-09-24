#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <stdint.h>

// Which pin on the Arduino is connected to the NeoPixels?
// On a Trinket or Gemma we suggest changing this to 1:
#define LED_PIN D0

// if high, the led power will be on, does not turn on the leds
#define LED_POWER_PIN D1

// where is the button
#define BUTTON_PIN D2

// battery charge pin: max voltage should be 3V
#define BATTERY_CHARGE_PIN D3



// How many leds are attached to the Arduino?
const uint16_t LED_COUNT = 29;

#endif
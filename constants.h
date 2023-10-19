#ifndef CONSTANTS_H
#define CONSTANTS_H

 #ifdef ARDUINO
 #if (ARDUINO >= 100)
 #include <Arduino.h>
 #else
 #include <WProgram.h>
 #include <pins_arduino.h>
 #endif
 
 #ifdef USE_TINYUSB // For Serial when selecting TinyUSB
 #include <Adafruit_TinyUSB.h>
 #endif
 
 #endif
 
 #ifdef TARGET_LPC1768
 #include <Arduino.h>
 #endif
 
 #if defined(ARDUINO_ARCH_RP2040)
 #include <stdlib.h>
 #include "hardware/pio.h"
 #include "hardware/clocks.h"
 #include "rp2040_pio.h"
 #endif

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

const uint32_t LOOP_UPDATE_PERIOD = 3;   // milliseconds (real values are more around 2 but that is ok)

#endif
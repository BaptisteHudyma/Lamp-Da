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

// Which pin on the Arduino is connected to the LED command pin
#define LED_PIN D2

// The pin that triggers the power delivery to the LEDS.
#define LED_POWER_PIN D7


// The button pin (one button pin to GND, the other to this pin)
#define BUTTON_PIN D0
// Pins for the led on the button
#define BUTTON_RED D3
#define BUTTON_GREEN D4
#define BUTTON_BLUE D5

// battery charge pin: max voltage should be 3V
#define BATTERY_CHARGE_PIN A1



// How many leds are attached to the Arduino?
const uint16_t LED_COUNT = 619;

const uint32_t LOOP_UPDATE_PERIOD = 40;   // milliseconds (average): depends of the number of leds

#endif
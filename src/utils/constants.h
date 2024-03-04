#ifndef CONSTANTS_H
#define CONSTANTS_H

 #include <cstdint>
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


// parameters of the led strip used
constexpr uint16_t ledByMeter = 160.0;      // the indexable led by meters
constexpr float ledStripWidth_mm = 5.2;     // width of the led strip

// parameters of the lamp body
constexpr uint16_t LED_COUNT = 618;         // How many indexable leds are attached to the controler
constexpr float lampBodyRadius_mm = 25;   // external radius of the lamp body 


// physical parameters computations
constexpr float ledSize_mm = 1.0 / ledByMeter * 1000.0;  // size of the individual led
constexpr float lampBodyCircumpherence_mm = 2.0 * 3.14159265 * lampBodyRadius_mm;
constexpr float ledStripLenght_mm = LED_COUNT * ledSize_mm;

constexpr float stripXCoordinates = lampBodyCircumpherence_mm / ledSize_mm + 0.35;
constexpr float stripYCoordinates = ledStripLenght_mm / lampBodyCircumpherence_mm;

constexpr float lampBodyHeight_mm = stripYCoordinates  * ledStripWidth_mm;


// compute the expected average loop runtime, scaled with the number of led +25% for computations
constexpr uint32_t LOOP_UPDATE_PERIOD = ceil(1.25 * (0.0483333 * LED_COUNT + 1.5983333));   // milliseconds (average): depends of the number of leds

#endif
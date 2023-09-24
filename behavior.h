#ifndef BEHAVIOR_HPP
#define BEHAVIOR_HPP

#include <Adafruit_NeoPixel.h>
#include "constants.h"

#ifdef __AVR__
#include <avr/power.h>  // Required for 16 MHz Adafruit Trinket
#endif

// Declare our NeoPixel strip object:
extern Adafruit_NeoPixel strip;

// NeoPixel brightness, 0 (min) to 255 (max)
extern uint8_t BRIGHTNESS;

// if false, the led strip should be deactivated
extern bool isActivated;

/**
 * \brief main update loop
 */
void color_mode_update();

/**
 * \brief callback of the button clicked sequence event 
 */
void button_clicked_callback(uint8_t consecutiveButtonCheck);

/**
 * \brief callback of the button clicked sequence event with a final hold
 */
void button_hold_callback(uint8_t consecutiveButtonCheck, uint32_t buttonHoldDuration);

#endif
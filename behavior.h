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
 * \brief update displayed color
 */
void colorDisplay();

/**
 * \brief callback of the button clicked sequence event 
 */
void buttonClickedCallback(uint8_t consecutiveButtonCheck);

/**
 * \brief callback of the button clicked sequence event with a final hold
 */
void buttonHoldCallback(uint8_t consecutiveButtonCheck, uint32_t buttonHoldDuration);

#endif
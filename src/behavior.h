#ifndef BEHAVIOR_HPP
#define BEHAVIOR_HPP

#include "utils/strip.h"
#include "utils/constants.h"

#ifdef __AVR__
#include <avr/power.h>  // Required for 16 MHz Adafruit Trinket
#endif

// Declare our NeoPixel strip object:
extern LedStrip strip;

// NeoPixel brightness, 0 (min) to 255 (max)
extern uint8_t BRIGHTNESS;

/**
 * \brief Load the parameters from the filesystem 
 */
void read_parameters();
void write_parameters();

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

// If any alert is set, will handle it
void handle_alerts();

#endif
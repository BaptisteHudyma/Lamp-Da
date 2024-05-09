#ifndef BEHAVIOR_HPP
#define BEHAVIOR_HPP

#include "alerts.h"
#include "utils/constants.h"

#ifdef __AVR__
#include <avr/power.h>  // Required for 16 MHz Adafruit Trinket
#endif

// NeoPixel brightness, 0 (min) to 255 (max)
extern uint8_t BRIGHTNESS;

/**
 * \brief Load the parameters from the filesystem
 */
void read_parameters();
void write_parameters();

bool is_shutdown();

// start sequence that check the battery level and start the leds, and power all
// systems
void startup_sequence();

// put in shutdown mode, with external wakeup
void shutdown();

void raise_battery_alert();

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
void button_hold_callback(uint8_t consecutiveButtonCheck,
                          uint32_t buttonHoldDuration);

// If any alert is set, will handle it
void handle_alerts();

#endif
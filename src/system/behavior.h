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
extern void read_parameters();
extern void write_parameters();

extern bool is_shutdown();

// start sequence that check the battery level and start the leds, and power all
// systems
extern void startup_sequence();

// put in shutdown mode, with external wakeup
extern void shutdown();

extern void update_brightness(const uint8_t newBrightness,
                              const bool shouldUpdateCurrentBrightness = false,
                              const bool isInitialRead = false);

/**
 * \brief callback of the button clicked sequence event
 */
extern void button_clicked_callback(const uint8_t consecutiveButtonCheck);

/**
 * \brief callback of the button clicked sequence event with a final hold
 */
extern void button_hold_callback(const uint8_t consecutiveButtonCheck,
                                 const uint32_t buttonHoldDuration);

/**
 * \brief disable button usermode UI and force the use of basic UI
 */
extern void button_disable_usermode();

/**
 * \brief return true if button usermode UI is enabled
 */
extern bool is_button_usermode_enabled();

// If any alert is set, will handle it
extern void handle_alerts();

#endif

#ifndef BEHAVIOR_HPP
#define BEHAVIOR_HPP

/** \file behavior.h
 *  \brief Basic controller behavior, including alerts and user interactions
 **/

#include "alerts.h"
#include "utils/constants.h"

#ifdef __AVR__
#include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif

/// BRIGHTNESS from NeoPixel brightness, from 0 (min) to 255 (max)
extern uint8_t BRIGHTNESS;

/// Read lamp configured parameters from filesystem
extern void read_parameters();

/// Write lamp configured parameters to filesystem
extern void write_parameters();

/// Return True if lamp power is off
extern bool is_shutdown();

/// Startup sequence (check battery, power all systems, light up LEDs)
extern void startup_sequence();

/// Shutdown sequence (shutdown all systems, disable LEDs, exit gracefully)
extern void shutdown();

/** \brief Function to call in order to update the lamp brightness
 *
 * \param[in] newBrightness The new target brightness to use
 * \param[in] shouldUpdateCurrentBrightness If True, update internal brightness
 * tracking to make the change permanent
 * \param[in] isInitialRead If True, do no propagate brightness change event to
 * user::brightness_update()
 */
extern void update_brightness(const uint8_t newBrightness,
                              const bool shouldUpdateCurrentBrightness = false,
                              const bool isInitialRead = false);

/// Main callback called to handle "button clicked" sequence event
extern void button_clicked_callback(const uint8_t consecutiveButtonCheck);

/// Main callback called to handle a "button clicked, then held" sequence event
extern void button_hold_callback(const uint8_t consecutiveButtonCheck,
                                 const uint32_t buttonHoldDuration);

/// Returns true if button "usermode UI" was enabled by user at startup
extern bool is_button_usermode_enabled();

/// Disable button "usermode UI" and return to default behaviors
extern void button_disable_usermode();

/// Handle alerts, update the button color, check the charger status
extern void handle_alerts();

#endif

#ifndef BEHAVIOR_HPP
#define BEHAVIOR_HPP

/** \file behavior.h
 *  \brief Basic controller behavior, including alerts and user interactions
 **/

#include "alerts.h"
#include "utils/constants.h"
#include "utils/utils.h"

#ifdef __AVR__
#include <avr/power.h>  // Required for 16 MHz Adafruit Trinket
#endif

// NeoPixel brightness, 0 (min) to 255 (max)
extern uint8_t BRIGHTNESS;

/// First ever boot flag for this lamp
static constexpr uint32_t isFirstBootKey = utils::hash("ifb");

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

/// Main callback to handle "button clicked" sequence event
extern void button_clicked_callback(const uint8_t consecutiveButtonCheck);

/// Main callback to handle a "button clicked, then held" sequence event
extern void button_hold_callback(const uint8_t consecutiveButtonCheck,
                                 const uint32_t buttonHoldDuration);

// If any alert is set, will handle it
extern void handle_alerts();

/// \private Internal symbol used to signify which LMBD_LAMP_TYPE was specified
#ifdef LMBD_CPP17
extern const char* ensure_build_canary();
#endif

#endif

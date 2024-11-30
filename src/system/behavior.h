#ifndef BEHAVIOR_HPP
#define BEHAVIOR_HPP

/** \file
 *
 *  \brief Basic controller behavior, including alerts and user interactions
 **/

#ifdef __AVR__
#include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif

#include "src/compile.h"
#include "src/system/alerts.h"
#include "src/system/utils/constants.h"
#include "src/system/utils/utils.h"

namespace behavior {

// NeoPixel brightness, 0 (min) to 255 (max)
extern uint8_t BRIGHTNESS;

/// First ever boot flag for this lamp
static constexpr uint32_t isFirstBootKey = utils::hash("ifb");

/**
 * \brief Load the parameters from the filesystem
 */
extern void read_parameters();

/**
 * \return true if the user code is runing
 */
extern bool is_user_code_running();

extern void update_brightness(const uint8_t newBrightness,
                              const bool shouldUpdateCurrentBrightness = false,
                              const bool isInitialRead = false);

/// Main callback to handle "button clicked" sequence event
extern void button_clicked_callback(const uint8_t consecutiveButtonCheck);

/// Main callback to handle a "button clicked, then held" sequence event
extern void button_hold_callback(const uint8_t consecutiveButtonCheck, const uint32_t buttonHoldDuration);

/// Disable button usermode UI
extern void button_disable_usermode();

/// Returns True only if button usermode UI is enabled
extern bool is_button_usermode_enabled();

/**
 * \brief call once at program start, before the first main loop runs
 * The system was powered from a vbus powered event, not a user action
 */
extern void set_woke_up_from_vbus(const bool wokeUp);

// main loop of the system
extern void loop();

} // namespace behavior

/// \private Internal symbol used to signify which LMBD_LAMP_TYPE was specified
#ifdef LMBD_CPP17
extern const char* ensure_build_canary();
#endif

#endif

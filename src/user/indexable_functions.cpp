#ifdef LMBD_LAMP_TYPE__INDEXABLE
#ifndef LMBD_SIMPLE_EMULATOR

// #ifndef NUDZ_MODES_ENABLED
// #define NUDZ_MODES_ENABLED
// #endif

#include <cstdint>

#include "src/system/logic/behavior.h"

#include "src/system/platform/gpio.h"

#include "src/user/functions.h"

//
// code below requires c++17
//

#ifdef LMBD_CPP17

#include "src/modes/include/group_type.hpp"
#include "src/modes/include/manager_type.hpp"

#include "src/modes/default/fixed_modes.hpp"
#include "src/modes/default/fireplace.hpp"
#include "src/modes/legacy/legacy_modes.hpp"

#ifdef NUDZ_MODES_ENABLED
#include "src/modes/custom/nudz/nudz_scrollimage.hpp"
#endif

namespace user {

//
// list your groups & modes here
//

#ifdef NUDZ_MODES_ENABLED
using NudzModes = modes::GroupFor<modes::custom::nudz::NudzHeinekenMode,
                                  modes::custom::nudz::NudzHuitSixMode,
                                  modes::custom::nudz::NudzViolonsaoulsMode,
                                  modes::custom::nudz::NudzBeerGlassMode>;

using ManagerTy = modes::ManagerFor<modes::FixedModes,
                                    // modes::MiscFixedModes,
                                    modes::legacy::CalmModes,
                                    modes::legacy::PartyModes,
                                    modes::legacy::SoundModes,
                                    NudzModes>;
#else
using ManagerTy = modes::ManagerFor<modes::FixedModes,
                                    // modes::MiscFixedModes,
                                    modes::legacy::CalmModes,
                                    modes::legacy::PartyModes,
                                    modes::legacy::SoundModes>;
#endif

//
// implementation details
//

namespace _private {

// The button pin (one button pin to GND, the other to this pin)
constexpr DigitalPin::GPIO ledStripPinId = DigitalPin::GPIO::gpio6;
static DigitalPin LedStripPin(ledStripPinId);

LedStrip strip(LedStripPin.pin());
modes::hardware::LampTy lamp {strip};
ManagerTy modeManager(lamp);

} // namespace _private

static auto get_context() { return user::_private::modeManager.get_context(); }

//
// indexable lamp is implemented in another castle
//

#include "src/modes/user/default_behavior.hpp"   // default manager callbacks
#include "src/modes/user/indexable_behavior.hpp" // custom RGB UI of the lamp

} // namespace user

#else
#warning "This file requires --std=gnu++17 or higher to build!*"
//
// *if you got this warning, check Makefile to see how to build project

//
// no c++17 support -- placeholder implementation
//

namespace user {

void power_on_sequence() {}
void power_off_sequence() {}

void brightness_update(const brightness_t) {}
void sunset_timer_update(const float progress) {}
void write_parameters() {}
void read_parameters() {}
void button_clicked_default(const uint8_t) {}
void button_hold_default(const uint8_t, const bool, const uint32_t) {}

bool button_start_click_default(const uint8_t clicks) { return false; }
bool button_start_hold_default(const uint8_t clicks, const bool isEndOfHoldEvent, const uint32_t holdDuration)
{
  return false;
}

bool button_clicked_usermode(const uint8_t) { return false; }
bool button_hold_usermode(const uint8_t, const bool, const uint32_t) { return false; }

void loop() {}
bool should_spawn_thread() { return false; }
void user_thread() {}

} // namespace user

#endif // LMBD_CPP17

#endif // NOT LMBD_SIMPLE_EMULATOR
#endif // LMBD_LAMP_TYPE__INDEXABLE

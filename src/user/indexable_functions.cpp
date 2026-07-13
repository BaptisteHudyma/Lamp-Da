#ifdef LMBD_LAMP_TYPE__INDEXABLE
#ifndef LMBD_SIMPLE_EMULATOR

/// Add the nudz mode the compilation
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
#include "src/modes/legacy/bluetooth_group.hpp"

#include "src/modes/custom/nudz/nudz_scrollimage.hpp"

namespace lampda::user {

//
// list your groups & modes here
//

/// Custom user mode groups
namespace custom {
using NudzModes = modes::GroupFor<modes::custom::nudz::NudzHeinekenMode,
                                  modes::custom::nudz::NudzHuitSixMode,
                                  modes::custom::nudz::NudzViolonsaoulsMode,
                                  modes::custom::nudz::NudzBeerGlassMode>;
}

using ManagerTy = modes::ManagerForHiddenGroups<
#ifdef NUDZ_MODES_ENABLED
        1, // BluetoothModes is defined as an hidden group
#else
        2, // NudzModes and BluetoothModes are defined as an hidden groups
#endif
        modes::FixedModes,
        modes::legacy::CalmModes,
        modes::legacy::PartyModes,
        modes::legacy::SoundModes,
        custom::NudzModes,
        modes::bluetooth::BluetoothModes>;

//
// implementation details
//

namespace _private {

// The button pin (one button pin to GND, the other to this pin)
constexpr platform::gpio::DigitalPin::GPIO ledStripPinId = platform::gpio::DigitalPin::GPIO::gpio6;
static platform::gpio::DigitalPin LedStripPin(ledStripPinId);

physical::LedStrip strip(LedStripPin.pin());
modes::hardware::LampTy lamp {strip};
ManagerTy modeManager(lamp);

} // namespace _private

static auto get_context() { return user::_private::modeManager.get_context(); }

} // namespace lampda::user

//
// indexable lamp is implemented in another castle
//

#include "src/modes/user/default_behavior.hpp"   // default manager callbacks
#include "src/modes/user/indexable_behavior.hpp" // custom RGB UI of the lamp

#else
#warning "This file requires --std=gnu++17 or higher to build!*"
//
// *if you got this warning, check Makefile to see how to build project

//
// no c++17 support -- placeholder implementation
//

namespace lampda::user {

void power_on_sequence() {}
void power_off_sequence() {}

void brightness_update(const brightness_t) {}
void sunset_timer_update(const float progress) {}
void write_parameters() {}
void read_parameters() {}
void button_clicked_default(const uint8_t) {}
bool button_hold_default(const uint8_t, const bool, const uint32_t) { return false; }

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

void handle_elk_command(const utils::ELK::Package&) {}

} // namespace lampda::user

#endif // LMBD_CPP17

#endif // NOT LMBD_SIMPLE_EMULATOR
#endif // LMBD_LAMP_TYPE__INDEXABLE

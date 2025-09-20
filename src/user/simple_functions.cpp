#ifdef LMBD_LAMP_TYPE__SIMPLE

#include <cstdint>

#include "src/system/logic/behavior.h"
#include "src/user/functions.h"

//
// code below requires c++17
//

#ifdef LMBD_CPP17

#include "src/modes/include/group_type.hpp"
#include "src/modes/include/manager_type.hpp"

#include "src/modes/default/brightness_modes.hpp"

namespace user {
//
// list your groups & modes here
//

using ManagerTy = modes::
        ManagerFor<modes::brightness::StaticLightOnly, modes::brightness::CalmGroup, modes::brightness::FlashesGroup>;

//
// implementation details
//

namespace _private {

modes::hardware::LampTy lamp {};
ManagerTy modeManager(lamp);

} // namespace _private

static auto get_context() { return user::_private::modeManager.get_context(); }

//
// simple lamp is implemented in another castle
//

#include "src/modes/user/default_behavior.hpp" // default manager callbacks
#include "src/modes/user/simple_behavior.hpp"  // custom RGB UI of the lamp

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
void write_parameters() {}
void read_parameters() {}
void button_clicked_default(const uint8_t) {}
void button_hold_default(const uint8_t, const bool, const uint32_t) {}

bool button_clicked_usermode(const uint8_t) { return false; }

bool button_hold_usermode(const uint8_t, const bool, const uint32_t) { return false; }

void loop() {}
bool should_spawn_thread() { return false; }
void user_thread() {}

} // namespace user

#endif // LMBD_CPP17

#endif

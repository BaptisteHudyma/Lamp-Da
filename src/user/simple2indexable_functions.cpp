#ifdef LMBD_LAMP_TYPE__INDEXABLE
#ifdef LMBD_SIMPLE_EMULATOR

#include <cstdint>

#include "src/system/behavior.h"
#include "src/user/functions.h"

#include "src/system/utils/utils.h"
#include "src/system/utils/brightness_handle.h"
#include "src/system/utils/curves.h"
#include "src/system/utils/print.h"

#include "src/system/platform/time.h"

#include "src/system/physical/fileSystem.h"
#include "src/system/physical/output_power.h"

namespace _sublamp {

#define LMBD_LAMP_TYPE__SIMPLE
#include "src/user/simple_functions.cpp"
#undef LMBD_LAMP_TYPE__SIMPLE

}

//
// code below requires c++17
//

#ifdef LMBD_CPP17

#include "src/modes/include/group_type.hpp"
#include "src/modes/include/manager_type.hpp"

#include "src/modes/default/fixed_modes.hpp"
#include "src/modes/default/fireplace.hpp"
#include "src/modes/legacy/legacy_modes.hpp"

namespace user {

//
// -- begin: implem details taken from indexable
//

namespace _private {

constexpr DigitalPin::GPIO ledStripPinId = DigitalPin::GPIO::gpio6;
static DigitalPin LedStripPin(ledStripPinId);
LedStrip strip(LedStripPin.pin());

} // namespace _private

//
// -- end
//

//
// proxy simple2indexable
//

void power_on_sequence() {
  _sublamp::user::power_on_sequence();
  ensure_build_canary();
}

void power_off_sequence()
{
  _sublamp::user::power_off_sequence();
}

void brightness_update(const brightness_t brightness)
{
  _sublamp::user::brightness_update(brightness);
  _private::strip.setBrightness((255 * brightness) / maxBrightness);
}

void write_parameters() {
  _sublamp::user::write_parameters();
}

void read_parameters() {
  _sublamp::user::read_parameters();
}

void button_clicked_default(const uint8_t clicks)
{
  _sublamp::user::button_clicked_default(clicks);
}

void button_hold_default(const uint8_t a, const bool b, const uint32_t c)
{
  _sublamp::user::button_hold_default(a, b, c);
}

bool button_clicked_usermode(const uint8_t a)
{
  return _sublamp::user::button_clicked_usermode(a);
}

bool button_hold_usermode(const uint8_t a, const bool b, const uint32_t c)
{
  return _sublamp::user::button_hold_usermode(a, b, c);
}

void loop()
{
  for (size_t I = 0; I < LED_COUNT; ++I)
  {
    _private::strip.setPixelColor(I, 0xff, 0xff, 0);
  }

  _sublamp::user::loop();
}

bool should_spawn_thread()
{
  return _sublamp::user::should_spawn_thread();
}

void user_thread()
{
  return _sublamp::user::user_thread();
}

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

#endif // LMBD_SIMPLE_EMULATOR
#endif // LMBD_LAMP_TYPE__INDEXABLE

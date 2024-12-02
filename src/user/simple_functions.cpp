#ifdef LMBD_LAMP_TYPE__SIMPLE

#include "functions.h"

#include "src/system/behavior.h"
#include "src/system/physical/fileSystem.h"
#include "src/system/physical/led_power.h"
#include "src/system/utils/utils.h"

namespace user {

void power_on_sequence() { ledpower::write_brightness(behavior::BRIGHTNESS); }

void power_off_sequence()
{
#ifdef LMBD_CPP17
  ensure_build_canary(); // (no-op) internal symbol used during build
#endif
}

void brightness_update(const uint8_t brightness)
{
  if (brightness == 255)
  {
    // blip
    ledpower::write_brightness(0);
    delay(4); // blip light off if we reached max level
  }
  ledpower::write_brightness(brightness);
}

void write_parameters() {}

void read_parameters() {}

void button_clicked_default(const uint8_t clicks)
{
  switch (clicks)
  {
    // put luminosity to maximum
    case 2:
      behavior::update_brightness(255, true);
      break;

    default:
      break;
  }
}

void button_hold_default(const uint8_t, const bool, const uint32_t) {}

bool button_clicked_usermode(const uint8_t) { return usermodeDefaultsToLockdown; }

bool button_hold_usermode(const uint8_t, const bool, const uint32_t) { return usermodeDefaultsToLockdown; }

void loop() {}

bool should_spawn_thread() { return false; }

void user_thread() {}

} // namespace user

#endif

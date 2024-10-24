#ifdef LMBD_LAMP_TYPE__SIMPLE

#include "functions.h"

#include "../../system/behavior.h"
#include "../../system/physical/led_power.h"

namespace user {

void power_on_sequence() { ledpower::write_brightness(BRIGHTNESS); }

void power_off_sequence() {}

void brightness_update(const uint8_t brightness) {
  if (brightness == 255) {
    // blip
    ledpower::write_brightness(0);
    delay(4);  // blip light off if we reached max level
  }
  ledpower::write_brightness(brightness);
}

void write_parameters() {}

void read_parameters() {}

void button_clicked(const uint8_t clicks) {
  switch (clicks) {
    case 0:
    case 1:
      break;

    case 2:
      // put luminosity to maximum
      update_brightness(255, true);
      break;

    default:
      // nothing
      break;
  }
}

void button_hold(const uint8_t clicks, const bool isEndOfHoldEvent,
                 const uint32_t holdDuration) {}

void loop() {}

}  // namespace user

#endif
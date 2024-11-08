#include "power_source.h"

#include "FUSB302/PD_UFP.h"

namespace powerSource {

// Create an instance of the PD negociation
PD_UFP_c PD_UFP;

// set to true when a power source is detected
inline static bool isPowerSourceDetected_s = false;
// set to the time when the source is detected
inline static uint32_t powerSourceDetectedTime_s = 0;

// check if the charger can use the max power
bool can_use_PD_full_power() {
  // voltage on VBUS is greater than negociated voltage - threshold (50 ms
  // steps)
  return PD_UFP.get_vbus_voltage() > (PD_UFP.get_voltage() * 50 - 2000);
}

// check if the source had time to stabilize
bool is_vbus_stable() {
  return isPowerSourceDetected_s and
         // let some time pass after a new detection
         millis() - powerSourceDetectedTime_s > 3000;
}

/**
 *
 *      HEADER FUNCTIONS BELOW
 *
 */

bool setup() {
  // initialize the power delivery process
  PD_UFP.init(CHARGE_INT, PD_POWER_OPTION_MAX_20V);
  return true;
}

void loop() {
  // run pd process
  PD_UFP.run();

  // source detected
  if (PD_UFP.is_vbus_ok()) {
    if (not isPowerSourceDetected_s) {
      powerSourceDetectedTime_s = millis();
      isPowerSourceDetected_s = true;
    }
  } else {
    isPowerSourceDetected_s = false;
  }
}

uint16_t get_max_input_current() {
  // vbus had time to stabilize
  if (is_vbus_stable()) {
    // power delivery is here
    const bool isUsbPD = is_usb_power_delivery();
    // power is available
    if (isUsbPD and can_use_PD_full_power()) {
      // resolution of 10 mA
      return PD_UFP.get_current() * 10;
    }
  }
  // standard USB current
  return 100;
}

bool is_usb_power_delivery() { return PD_UFP.is_USB_PD_available(); }

bool is_power_available() { return isPowerSourceDetected_s; }

}  // namespace powerSource
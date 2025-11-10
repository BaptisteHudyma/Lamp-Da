#include "power_gates.h"

#include "src/system/platform/gpio.h"
#include "src/system/platform/time.h"
#include "src/system/platform/print.h"

#include <cassert>

namespace powergates {

bool isInit = false;

// if this is too low, there is a risk that both gate can be active at the same time
// potentially destructive behavior
constexpr uint32_t gateSwitchDelay_ms = 50;

// wake up time for a blip output gate. DANGER
static uint32_t blipWakeUpTime = 0;

// indicates if the power gate is really on, not switching or other
bool isPowerGateReallyEnabled = false;

namespace __private {
const DigitalPin enableVbusGate(DigitalPin::GPIO::Output_EnableVbusGate);
const DigitalPin enablePowerGate(DigitalPin::GPIO::Output_EnableOutputGate);
} // namespace __private

void enable_gate(bool isVbusGate)
{
  const bool isVbusGateEnabled = isVbusGate;
  const bool isPowerGateEnabled = not isVbusGate;

  // force gates to have opposite states
  __private::enablePowerGate.set_high(isPowerGateEnabled);
  __private::enableVbusGate.set_high(isVbusGateEnabled);

  // set real status
  delay_ms(1);
  isPowerGateReallyEnabled = isPowerGateEnabled;

  if (isPowerGateEnabled)
    lampda_print("power gate enabled");
  else
    lampda_print("vbus gate enabled");
}

bool isVbusGateEnabled = false;
bool isVbusGateSwitching = false;
uint32_t vbusGateStartSwitchingTime = 0;

bool isPowerGateEnabled = false;
bool isPowerGateSwitching = false;
uint32_t powerGateStartSwitchingTime = 0;

void disable_vbus_gate()
{
  if (__private::enableVbusGate.is_high())
    lampda_print("vbus gate disabled");

  __private::enableVbusGate.set_high(false);
  isVbusGateEnabled = false;
  isVbusGateSwitching = false;
}

void disable_power_gate()
{
  if (__private::enablePowerGate.is_high())
    lampda_print("power gate disabled");

  __private::enablePowerGate.set_high(false);
  isPowerGateEnabled = false;
  isPowerGateSwitching = false;
}

void switch_delayed_vbus_gate(bool enable)
{
  if (enable != isVbusGateEnabled)
  {
    vbusGateStartSwitchingTime = time_ms();
    isVbusGateSwitching = true;
    isVbusGateEnabled = enable;
  }
}

void switch_delayed_power_gate(bool enable)
{
  if (enable != isPowerGateEnabled)
  {
    powerGateStartSwitchingTime = time_ms();
    isPowerGateSwitching = true;
    isPowerGateEnabled = enable;
  }
}

bool is_power_gate_switched() { return time_ms() - powerGateStartSwitchingTime > gateSwitchDelay_ms; }
bool is_vbus_gate_switched() { return time_ms() - vbusGateStartSwitchingTime > gateSwitchDelay_ms; }

bool is_power_gate_enabled() { return isPowerGateReallyEnabled; }

void init()
{
  disable_gates();
  isInit = true;
}

void loop()
{
  if (!isInit)
    return;

  // vbus is switching
  if (isVbusGateSwitching and is_vbus_gate_switched())
  {
    enable_gate(true);
    isVbusGateSwitching = false;
  }
  if (isPowerGateSwitching and is_power_gate_switched())
  {
    enable_gate(false);
    isPowerGateSwitching = false;
  }

  // allow blipping
  // This is dangerous for the hardware, dont touch if you dont know how
  if (isPowerGateReallyEnabled)
  {
    // prepare blip wake up
    if (blipWakeUpTime != 0 && time_ms() >= blipWakeUpTime)
    {
      // REALLY DANGEROUS STUFF : enable power gate without checks
      __private::enablePowerGate.set_high(true);
      // reset time
      blipWakeUpTime = 0;
    }
  }
  else
  {
    // always reset if power gate is inactive
    blipWakeUpTime = 0;
  }
}

namespace power {

void blip(const uint32_t timing)
{
#ifdef LMBD_LAMP_TYPE__INDEXABLE
  // NEVER EVER BLIP THE INDEXABLE, the strip is too weak
  assert(false);
#endif

  // hard limit on blip
  if (timing <= 0 or timing > 1000 or not isPowerGateReallyEnabled)
  {
    blipWakeUpTime = 0;
    return;
  }

  blipWakeUpTime = time_ms() + timing;

  // deactivate without checks, not that dangerous...
  __private::enablePowerGate.set_high(false);
}

} // namespace power

bool __is_power_gate_enabled() { return isPowerGateEnabled and is_power_gate_switched(); }

void enable_power_gate()
{
  if (__is_power_gate_enabled())
    return;
  disable_vbus_gate();
  switch_delayed_power_gate(true);
}

void enable_vbus_gate()
{
  if (is_vbus_gate_enabled())
    return;
  disable_power_gate();
  switch_delayed_vbus_gate(true);
}

void enable_vbus_gate_DIRECT()
{
  disable_power_gate();
  enable_gate(true);
}

bool is_vbus_gate_enabled() { return isVbusGateEnabled and is_vbus_gate_switched(); }

void disable_gates()
{
  disable_vbus_gate();
  disable_power_gate();
}

bool are_gate_disabled() { return not is_vbus_gate_enabled() and not __is_power_gate_enabled(); }

} // namespace powergates

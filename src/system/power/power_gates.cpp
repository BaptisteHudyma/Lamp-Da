#include "power_gates.h"

#include "src/system/platform/gpio.h"
#include "src/system/platform/time.h"

namespace powergates {

bool isInit = false;

constexpr uint32_t gateSwitchDelay_ms = 100;

namespace __private {
DigitalPin enableVbusGate(DigitalPin::GPIO::Output_EnableVbusGate);
DigitalPin enablePowerGate(DigitalPin::GPIO::Output_EnableOutputGate);
} // namespace __private

void enable_gate(bool isVbusGate)
{
  // force gates to have opposite states
  __private::enablePowerGate.set_high(!isVbusGate);
  __private::enableVbusGate.set_high(isVbusGate);
}

bool isVbusGateEnabled = false;
bool isVbusGateSwitching = false;
uint32_t vbusGateStartSwitchingTime = 0;

bool isPowerGateEnabled = false;
bool isPowerGateSwitching = false;
uint32_t powerGateStartSwitchingTime = 0;

void disable_vbus_gate()
{
  __private::enableVbusGate.set_high(false);
  isVbusGateEnabled = false;
  isVbusGateSwitching = false;
}

void disable_power_gate()
{
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
}

void enable_power_gate()
{
  if (is_power_gate_enabled())
    return;
  disable_vbus_gate();
  switch_delayed_power_gate(true);
}

bool is_power_gate_enabled() { return isPowerGateEnabled and is_power_gate_switched(); }

void enable_vbus_gate()
{
  if (is_vbus_gate_enabled())
    return;
  disable_power_gate();
  switch_delayed_vbus_gate(true);
}

bool is_vbus_gate_enabled() { return isVbusGateEnabled and is_vbus_gate_switched(); }

void disable_gates()
{
  disable_vbus_gate();
  disable_power_gate();
}

bool are_gate_disabled() { return not is_vbus_gate_enabled() and not is_power_gate_enabled(); }

} // namespace powergates
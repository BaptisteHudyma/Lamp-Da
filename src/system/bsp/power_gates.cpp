#include "power_gates.h"

#include "src/system/hal/gpio.h"
#include "src/system/hal/time.h"

#include "src/system/bsp/text_out.h"

#include <atomic>
#include <cassert>

namespace lampda {
namespace bsp {
namespace powergates {

bool _isInit = false;

// if this is too low, there is a risk that both gate can be active at the same time
// potentially destructive behavior
static constexpr uint32_t gateSwitchDelay_ms = 50;

/// max off delay of the gate blip
static constexpr uint32_t BLIP_MAX_TIME = 10000;
// start time of a blip of the output gate. DANGER
// Written before _blipDuration_ms (release), read after it (acquire), as blip() and loop() run on different threads
static uint32_t _blipStartTime_ms = 0;
// duration of a blip of the output gate, 0 if no blip is active. DANGER
static std::atomic<uint32_t> _blipDuration_ms {0};

// indicates if the power gate is really on, not switching or other
bool _isPowerGateReallyEnabled = false;

namespace __private {
const hal::gpio::DigitalPin enableVbusGate(hal::gpio::DigitalPin::GPIO::Output_EnableVbusGate);
const hal::gpio::DigitalPin enablePowerGate(hal::gpio::DigitalPin::GPIO::Output_EnableOutputGate);
} // namespace __private

static bool _isVbusGateEnabled = false;
static bool _isVbusGateSwitching = false;
static uint32_t _vbusGateStartSwitchingTime = 0;

static bool _isPowerGateEnabled = false;
static bool _isPowerGateSwitching = false;
static uint32_t _powerGateStartSwitchingTime = 0;

///
bool is_power_gate_switched() { return hal::time_ms() - _powerGateStartSwitchingTime > gateSwitchDelay_ms; }
bool is_vbus_gate_switched() { return hal::time_ms() - _vbusGateStartSwitchingTime > gateSwitchDelay_ms; }

bool is_power_gate_enabled() { return _isPowerGateReallyEnabled; }

/// check if power gate is enabled
bool __is_power_gate_enabled() { return _isPowerGateEnabled and is_power_gate_switched(); }

void enable_gate(bool isVbusGate)
{
  const bool isVbusGateEnabled = isVbusGate;
  const bool isPowerGateEnabled = not isVbusGate;

  // force gates to have opposite states
  __private::enablePowerGate.set_high(isPowerGateEnabled);
  __private::enableVbusGate.set_high(isVbusGateEnabled);

  // set real status
  hal::delay_ms(1);

  if (_isPowerGateReallyEnabled != isPowerGateEnabled)
  {
    if (isPowerGateEnabled)
      bsp::lampda_print("power gate enabled");
    else
      bsp::lampda_print("vbus gate enabled");
  }
  _isPowerGateReallyEnabled = isPowerGateEnabled;
}

void disable_vbus_gate()
{
  if (__private::enableVbusGate.is_high())
    bsp::lampda_print("vbus gate disabled");

  __private::enableVbusGate.set_high(false);
  _isVbusGateEnabled = false;
  _isVbusGateSwitching = false;
}

void disable_power_gate()
{
  // reset blip time
  _blipDuration_ms = 0;

  if (__private::enablePowerGate.is_high())
    bsp::lampda_print("power gate disabled");

  __private::enablePowerGate.set_high(false);
  _isPowerGateEnabled = false;
  _isPowerGateSwitching = false;
}

void switch_delayed_vbus_gate(bool enable)
{
  if (enable != _isVbusGateEnabled)
  {
    _vbusGateStartSwitchingTime = hal::time_ms();
    _isVbusGateSwitching = true;
    _isVbusGateEnabled = enable;
  }
}

void switch_delayed_power_gate(bool enable)
{
  if (enable != _isPowerGateEnabled)
  {
    _powerGateStartSwitchingTime = hal::time_ms();
    _isPowerGateSwitching = true;
    _isPowerGateEnabled = enable;
  }
}

void init()
{
  disable_gates();
  _isInit = true;
}

void loop()
{
  if (!_isInit)
    return;

  // vbus is switching
  if (_isVbusGateSwitching and is_vbus_gate_switched())
  {
    enable_gate(true);
    _isVbusGateSwitching = false;
  }
  if (_isPowerGateSwitching and is_power_gate_switched())
  {
    enable_gate(false);
    _isPowerGateSwitching = false;
  }

  // allow blipping
  // This is dangerous for the hardware, dont touch if you dont know how
  if (_isPowerGateReallyEnabled)
  {
    // prepare blip wake up (elapsed time comparison is safe across a time_ms wrap)
    const uint32_t blipDuration_ms = _blipDuration_ms.load(std::memory_order_acquire);
    if (blipDuration_ms != 0 && (hal::time_ms() - _blipStartTime_ms) >= blipDuration_ms)
    {
      power::cancel_blip();
    }
  }
  else
  {
    // always reset if power gate is inactive
    _blipDuration_ms = 0;
  }
}

namespace power {

void blip(const uint32_t timing)
{
#ifdef LMBD_LAMP_TYPE__INDEXABLE
  // NEVER EVER BLIP THE INDEXABLE, the strip is too weak
  assert(false);
  return;
#endif

  // hard limit on blip
  if (timing <= 0 or timing > BLIP_MAX_TIME or not _isPowerGateReallyEnabled)
  {
    _blipDuration_ms = 0;
    return;
  }

  _blipStartTime_ms = hal::time_ms();
  _blipDuration_ms.store(timing, std::memory_order_release);

  // deactivate without checks, not that dangerous...
  __private::enablePowerGate.set_high(false);
}

void cancel_blip()
{
  // This is dangerous for the hardware, dont touch if you dont know how

  if (is_bliping() && _isPowerGateReallyEnabled)
  {
    // REALLY DANGEROUS STUFF : enable power gate without checks
    __private::enablePowerGate.set_high(true);
  }
  // reset time
  _blipDuration_ms = 0;
}

bool is_bliping() { return _blipDuration_ms > 0; }

} // namespace power

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

bool is_vbus_gate_enabled() { return _isVbusGateEnabled and is_vbus_gate_switched(); }

void disable_gates()
{
  disable_vbus_gate();
  disable_power_gate();
}

bool are_gate_disabled() { return not is_vbus_gate_enabled() and not __is_power_gate_enabled(); }

} // namespace powergates
} // namespace bsp
} // namespace lampda

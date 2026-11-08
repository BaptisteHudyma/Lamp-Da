#include "power_gates.h"

#include "src/system/hal/gpio.h"
#include "src/system/hal/mutex.h"
#include "src/system/hal/time.h"

#include "src/system/bsp/text_out.h"

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
// start time of a blip of the output gate. DANGER. Only access it under _powerGateMutex
static uint32_t _blipStartTime_ms = 0;
// duration of a blip of the output gate, 0 if no blip is active. DANGER. Only access it under _powerGateMutex
static uint32_t _blipDuration_ms = 0;

// indicates if the power gate is really on, not switching or other. Only access it under _powerGateMutex
static bool _isPowerGateReallyEnabled = false;

/// Guards the power gate pin, the blip state (_blipStartTime_ms, _blipDuration_ms) and _isPowerGateReallyEnabled:
/// blip() and cancel_blip() run on the user thread, while loop() and the gate switches run on the power thread.
/// A blip start, its timeout and its cancellation are then never interleaved with each other or with a gate switch.
/// Not recursive.
static hal::mutex::hal_mutex_t _powerGateMutex;
/// Initialized at program start: blips can be requested before the power init
[[maybe_unused]] static const bool _isPowerGateMutexInit = (hal::mutex::hal_mutex_init(&_powerGateMutex), true);

/// Hold _powerGateMutex for the scope
struct PowerGateLock
{
  PowerGateLock() { hal::mutex::hal_mutex_lock(&_powerGateMutex); }
  ~PowerGateLock() { hal::mutex::hal_mutex_unlock(&_powerGateMutex); }
  PowerGateLock(const PowerGateLock&) = delete;
  PowerGateLock& operator=(const PowerGateLock&) = delete;
};

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

bool is_power_gate_enabled()
{
  PowerGateLock lock;
  return _isPowerGateReallyEnabled;
}

/// check if power gate is enabled
bool __is_power_gate_enabled() { return _isPowerGateEnabled and is_power_gate_switched(); }

void enable_gate(bool isVbusGate)
{
  const bool isVbusGateEnabled = isVbusGate;
  const bool isPowerGateEnabled = not isVbusGate;

  bool hasChanged = false;
  {
    // a blip cancellation can not enable the power gate in the middle of this switch
    PowerGateLock lock;

    // a gate switch ends any blip
    _blipDuration_ms = 0;

    // force gates to have opposite states
    __private::enablePowerGate.set_high(isPowerGateEnabled);
    __private::enableVbusGate.set_high(isVbusGateEnabled);

    // set real status
    hal::delay_ms(1);

    hasChanged = _isPowerGateReallyEnabled != isPowerGateEnabled;
    _isPowerGateReallyEnabled = isPowerGateEnabled;
  }

  if (hasChanged)
  {
    if (isPowerGateEnabled)
      bsp::lampda_print("power gate enabled");
    else
      bsp::lampda_print("vbus gate enabled");
  }
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
  bool wasEnabled = false;
  {
    // a blip cancellation can not enable the power gate again after this
    PowerGateLock lock;
    // reset blip time
    _blipDuration_ms = 0;
    _isPowerGateReallyEnabled = false;

    wasEnabled = __private::enablePowerGate.is_high();
    __private::enablePowerGate.set_high(false);
  }

  if (wasEnabled)
    bsp::lampda_print("power gate disabled");

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

namespace power {
namespace __private {
void cancel_blip_locked();
} // namespace __private
} // namespace power

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
  // The timeout check and the gate update are done under the lock: a blip started meanwhile can not be erased
  PowerGateLock lock;
  if (_isPowerGateReallyEnabled)
  {
    // prepare blip wake up (elapsed time comparison is safe across a time_ms wrap)
    if (_blipDuration_ms != 0 && (hal::time_ms() - _blipStartTime_ms) >= _blipDuration_ms)
    {
      power::__private::cancel_blip_locked();
    }
  }
  else
  {
    // always reset if power gate is inactive
    _blipDuration_ms = 0;
  }
}

namespace power {

namespace __private {

/// cancel the current blip, if any. Call with _powerGateMutex held
void cancel_blip_locked()
{
  // This is dangerous for the hardware, dont touch if you dont know how

  if (_blipDuration_ms != 0 && _isPowerGateReallyEnabled)
  {
    // REALLY DANGEROUS STUFF : enable power gate without checks
    // The lock guarantees that the power gate was not disabled, nor the vbus gate enabled, since the check
    ::lampda::bsp::powergates::__private::enablePowerGate.set_high(true);
  }
  // reset time
  _blipDuration_ms = 0;
}

} // namespace __private

void blip(const uint32_t timing)
{
#ifdef LMBD_LAMP_TYPE__INDEXABLE
  // NEVER EVER BLIP THE INDEXABLE, the strip is too weak
  assert(false);
  return;
#endif

  PowerGateLock lock;

  // hard limit on blip
  if (timing <= 0 or timing > BLIP_MAX_TIME or not _isPowerGateReallyEnabled)
  {
    // a refused request ends the current blip: restore the output gate, or it would stay off with no blip to end
    // (only restored if the power gate is really enabled, see cancel_blip_locked)
    __private::cancel_blip_locked();
    return;
  }

  _blipStartTime_ms = hal::time_ms();
  _blipDuration_ms = timing;

  // deactivate without checks, not that dangerous...
  ::lampda::bsp::powergates::__private::enablePowerGate.set_high(false);
}

void cancel_blip()
{
  PowerGateLock lock;
  __private::cancel_blip_locked();
}

bool is_bliping()
{
  PowerGateLock lock;
  return _blipDuration_ms > 0;
}

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

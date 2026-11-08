/*! \file input_power_policy.h
    \brief Rules deciding when, and how much, current can be pulled from the USB input.

    Pure functions of a status snapshot, so that the rules can be unit tested without the PD hardware.
*/

#ifndef BSP_PD_INPUT_POWER_POLICY_H
#define BSP_PD_INPUT_POWER_POLICY_H

#include <cstdint>

namespace lampda {
namespace bsp {
namespace powerDelivery {
/// Input power rules, from a snapshot of the source status
namespace policy {

/// Delay after a source detection (or a PD resume) before considering it a standard (non PD) port
static constexpr uint32_t standardPortDetectionDelay_ms = 1500;
/// Delay after the last PD source detection before considering it a standard port
static constexpr uint32_t pdLostDelay_ms = 1000;
/**
 * After this delay, the standard port fallback does not wait for the PD negociation anymore.
 * It is only a guard against a stuck state machine: every negociation state has its own timeout.
 * Worst case of a late negociation: CC debounce (200ms) + caps wait (600ms) + soft reset (30ms)
 * + 2 * (hard reset recovery (650ms + 1275ms) + caps wait (600ms)) ~= 5.9s
 */
static constexpr uint32_t maxPdNegociationWait_ms = 8000;
/// Current pulled from a standard port (ICO finds the real maximum)
static constexpr uint16_t standardPortCurrent_mA = 800;
/// VBUS can be this much under the negociated voltage and still be considered established
static constexpr uint32_t pdVoltageMargin_mV = 1000;
/// Do not use the whole PD current, or the source will cut us off
static constexpr float pdCurrentRatio = 0.90f;

/// Snapshot of the input source status
struct SourceStatus
{
  /// VBUS was detected (deglitched)
  bool isPowerSourceDetected = false;
  /// Time since the source detection, or since the PD state machine resumed
  uint32_t timeSinceSourceDetected_ms = 0;
  /// Time since the source capabilities were last known
  uint32_t timeSinceLastPdDetected_ms = 0;
  /// Source capabilities are known
  bool isUsbPd = false;
  /// The PD state machine has an established contract
  bool isSinkReady = false;
  /// The PD state machine is still negociating (waiting for capabilities, resetting, requesting)
  bool isSinkNegotiating = false;
  /// Negociated contract (0 when no contract)
  uint32_t negotiatedVoltage_mV = 0;
  uint32_t negotiatedCurrent_mA = 0;
  /// Measured VBUS voltage
  uint32_t vbusVoltage_mV = 0;
};

/// The PD contract is established, and VBUS reached the negociated voltage
inline bool can_use_pd_full_power(const SourceStatus& s)
{
  return s.negotiatedVoltage_mV > 0 and s.isSinkReady and
         // the voltage/current rise should be over
         s.vbusVoltage_mV + pdVoltageMargin_mV >= s.negotiatedVoltage_mV;
}

/// The source is a standard USB port (no PD), with a stabilize delay
inline bool is_standard_port(const SourceStatus& s)
{
  return s.isPowerSourceDetected and
         // let some time pass after a new detection
         s.timeSinceSourceDetected_ms > standardPortDetectionDelay_ms and
         // last pd detected was some time ago
         s.timeSinceLastPdDetected_ms > pdLostDelay_ms and
         // a late PD negociation (soft/hard reset when the source caps were missed) can take seconds.
         // Pulling current from a fast charger before it ends can make it crash (issue #633)
         (s.timeSinceSourceDetected_ms > maxPdNegociationWait_ms or not s.isSinkNegotiating);
}

/// The source power can be used
inline bool can_use_power(const SourceStatus& s)
{
  return s.isPowerSourceDetected and ((s.isUsbPd and can_use_pd_full_power(s)) or is_standard_port(s));
}

/// Maximum current that can be pulled from the source
inline uint16_t max_input_current_mA(const SourceStatus& s)
{
  // safety
  if (not s.isPowerSourceDetected)
    return 0;

  // power delivery detected
  if (s.isUsbPd)
  {
    if (can_use_pd_full_power(s))
      return static_cast<uint16_t>(s.negotiatedCurrent_mA * pdCurrentRatio);
    // no contract yet
    return 0;
  }
  // no usb pd since some time, and vbus seems stable so try to use it
  if (is_standard_port(s))
    // maximum USB current (hacky, would prefer USB type detection)
    return standardPortCurrent_mA;

  // we dont known for now the type of connection
  return 0;
}

} // namespace policy
} // namespace powerDelivery
} // namespace bsp
} // namespace lampda

#endif

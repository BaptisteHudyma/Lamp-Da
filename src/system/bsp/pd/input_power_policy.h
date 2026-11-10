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
/// PD source output tolerance (vSrcNew), in percents of the negociated voltage
static constexpr uint32_t pdSourceTolerance_percent = 5;
/// Cable and connectors voltage drop allowed on VBUS (USB-C, at 3A)
static constexpr uint32_t pdCableDrop_mV = 500;
/// VBUS measure resolution: the FUSB302 measure has 420mV steps, and rounds down
static constexpr uint32_t vbusMeasureResolution_mV = 420;

/**
 * \brief Lowest VBUS measure for which the negociated voltage is considered established
 * A fixed 1V margin blocked a 9V contract from another lamp: VBUS read 7.98V (issue #357)
 */
inline uint32_t pd_min_established_vbus_mV(const uint32_t negotiatedVoltage_mV)
{
  const uint32_t margin_mV =
          negotiatedVoltage_mV * pdSourceTolerance_percent / 100 + pdCableDrop_mV + vbusMeasureResolution_mV;
  return negotiatedVoltage_mV > margin_mV ? negotiatedVoltage_mV - margin_mV : 0;
}
/// Do not use the whole PD current, or the source will cut us off
static constexpr float pdCurrentRatio = 0.90f;

/// VBUS can be lost this long before the source is considered unplugged (flicker, ICO algorithm)
static constexpr uint32_t unplugDeglitch_ms = 1000;
/**
 * During a hard reset, the source drops VBUS: tSafe0V (650ms) + tSrcRecover (1000ms) + tSrcTurnOn (275ms) ~= 1.9s.
 * VBUS can be lost this long before the source is considered unplugged. This limit does not depend on the PD state
 * machine: if it stalls in a hard reset state, an unplug must still be detected (issue #642).
 * Also used for a power role swap: old source turn off (tPSSourceOff, 920ms) + new source turn on (~400ms).
 */
static constexpr uint32_t hardResetUnplugDeglitch_ms = 3000;

/**
 * \brief True when the VBUS measure can come from our own OTG output, and is not a power source
 * The charger OTG also powers the output gate (output mode), and the two gates are never enabled together: the USB
 * port is then isolated from our output, and VBUS can only come from an external source. It must be followed, or a
 * cable unplugged in output mode is never seen (issue #646).
 * While the gates are switching (both disabled), the port can still hold our own voltage: it is ignored too.
 */
inline bool is_vbus_from_own_output(const bool isSwitchingToOtg,
                                    const bool isChargerInOtg,
                                    const bool isOutputGateEnabled)
{
  return isSwitchingToOtg or (isChargerInOtg and not isOutputGateEnabled);
}

/// VBUS was lost long enough to consider the source unplugged
inline bool is_unplugged(const uint32_t timeSinceVbusValid_ms, const bool isSinkHardResetting)
{
  return timeSinceVbusValid_ms > (isSinkHardResetting ? hardResetUnplugDeglitch_ms : unplugDeglitch_ms);
}

/**
 * Follows a VBUS loss, to decide when it is an unplug.
 * A hard reset seen during the loss keeps the hard reset grace until VBUS is valid again: the state machine can leave
 * the hard reset states before VBUS is back over our threshold, and that is not an unplug. The deadline is always
 * counted from the original VBUS loss, so no PD state change can extend it (issue #642).
 */
class VbusLossTracker
{
public:
  /// VBUS is valid: a new loss starts from this time
  void on_vbus_valid(const uint32_t time_ms)
  {
    lastVbusValid_ms_ = time_ms;
    wasHardResetSeen_ = false;
  }

  /// VBUS is not valid: return true if this loss is an unplug
  bool is_unplugged(const uint32_t time_ms, const bool isSinkHardResetting)
  {
    wasHardResetSeen_ = wasHardResetSeen_ or isSinkHardResetting;
    return policy::is_unplugged(time_ms - lastVbusValid_ms_, wasHardResetSeen_);
  }

private:
  uint32_t lastVbusValid_ms_ = 0;
  bool wasHardResetSeen_ = false;
};

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
         s.vbusVoltage_mV >= pd_min_established_vbus_mV(s.negotiatedVoltage_mV);
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

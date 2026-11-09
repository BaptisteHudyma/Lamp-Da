#ifndef LMBD_DRIVER_PD_CC_THRESHOLDS_H
#define LMBD_DRIVER_PD_CC_THRESHOLDS_H

/**
 * Source side CC detection thresholds, adapted to the Lamp-Da hardware (issue #357).
 *
 * The USB-C connector of all Lamp-Da boards has a permanent 5.1k pull-down (Rd) on both CC lines. When the FUSB302
 * pulls up the CC lines (we are a power source, OTG mode), each line sees this board Rd in parallel with what the
 * partner presents:
 *  - nothing attached on this line: board Rd alone, 5.1k
 *  - a sink Rd: board Rd || Rd, 2.55k
 *  - a sink with the same hardware flaw (another lamp): board Rd || Rd || its board Rd, 1.7k
 *  - a cable Ra (e-marker): board Rd || Ra, under 1k
 *
 * The standard thresholds detect both of the first cases as Rd: the lines were never seen as open, so all attachments
 * were taken for a debug accessory with a fixed CC1 polarity (no PD with the plug in the other orientation), and the
 * detachment was never detected.
 * These thresholds are placed between those equivalent resistances instead, and scaled by the Rp current source.
 *
 * This file is C compatible: it is used by the FUSB302 driver.
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/// Permanent pull-down on each CC line of the board, in ohms
#define LMBD_CC_BOARD_RD_OHM 5100u
/// Open vs Rd threshold, as the equivalent CC resistance: between board Rd alone (5.1k) and board Rd || Rd (2.55k)
#define LMBD_CC_OPEN_THRESHOLD_OHM 3800u
/// Rd vs Ra threshold, as the equivalent CC resistance: between board Rd || Rd || Rd (1.7k, a sink lamp) and
/// board Rd || Ra (< 1k).
/// Only reliable with the 1.5A and 3A Rp current sources: the default USB one has a too large tolerance (+/-20%)
#define LMBD_CC_RD_THRESHOLD_OHM 1300u

/// FUSB302 Rp current sources, in microamps
#define LMBD_FUSB302_RP_USB_UA 80u
#define LMBD_FUSB302_RP_1A5_UA 180u
#define LMBD_FUSB302_RP_3A0_UA 330u

/// FUSB302 measure DAC: the CC voltage is compared to (MDAC + 1) * 42mV
#define LMBD_FUSB302_MDAC_STEP_MV 42u
#define LMBD_FUSB302_MDAC_MAX     0x3fu

  /// Voltage developed by an Rp current source in a resistance, in millivolts (uA * ohm = uV)
  static inline uint32_t lmbd_cc_voltage_mv(const uint32_t rp_ua, const uint32_t resistance_ohm)
  {
    return (rp_ua * resistance_ohm + 500u) / 1000u;
  }

  /// MDAC value with the closest comparator threshold to a voltage
  static inline uint8_t lmbd_fusb302_mdac_from_mv(const uint32_t mv)
  {
    uint32_t steps = (mv + LMBD_FUSB302_MDAC_STEP_MV / 2u) / LMBD_FUSB302_MDAC_STEP_MV;
    if (steps < 1u)
      steps = 1u;
    if (steps > LMBD_FUSB302_MDAC_MAX + 1u)
      steps = LMBD_FUSB302_MDAC_MAX + 1u;
    return (uint8_t)(steps - 1u);
  }

  /// Comparator threshold of an MDAC value, in millivolts
  static inline uint32_t lmbd_fusb302_mdac_to_mv(const uint8_t mdac)
  {
    return ((uint32_t)(mdac & LMBD_FUSB302_MDAC_MAX) + 1u) * LMBD_FUSB302_MDAC_STEP_MV;
  }

  /// MDAC for the open vs Rd comparison: a CC line above it has nothing attached
  static inline uint8_t lmbd_cc_open_mdac(const uint32_t rp_ua)
  {
    return lmbd_fusb302_mdac_from_mv(lmbd_cc_voltage_mv(rp_ua, LMBD_CC_OPEN_THRESHOLD_OHM));
  }

  /// MDAC for the Rd vs Ra comparison: a CC line above it (and under the open threshold) has an Rd attached
  static inline uint8_t lmbd_cc_rd_mdac(const uint32_t rp_ua)
  {
    return lmbd_fusb302_mdac_from_mv(lmbd_cc_voltage_mv(rp_ua, LMBD_CC_RD_THRESHOLD_OHM));
  }

#ifdef __cplusplus
}
#endif

#endif

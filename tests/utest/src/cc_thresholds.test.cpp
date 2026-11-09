#include <gtest/gtest.h>

#include "src/system/driver/pd/cc_thresholds.h"

#include <cstdint>
#include <vector>

namespace {

enum class CcLevel
{
  Open,
  Rd,
  Ra
};

const char* to_str(const CcLevel level)
{
  switch (level)
  {
    case CcLevel::Open:
      return "open";
    case CcLevel::Rd:
      return "Rd";
    case CcLevel::Ra:
      return "Ra";
  }
  return "?";
}

/// Same comparisons as measure_cc_pin_source in FUSB302.c: the comparator is set when the CC voltage is above the MDAC
CcLevel classify(const double cc_mV, const uint32_t rp_uA)
{
  if (cc_mV > lmbd_fusb302_mdac_to_mv(lmbd_cc_open_mdac(rp_uA)))
    return CcLevel::Open;
  if (cc_mV > lmbd_fusb302_mdac_to_mv(lmbd_cc_rd_mdac(rp_uA)))
    return CcLevel::Rd;
  return CcLevel::Ra;
}

double parallel(const double a, const double b) { return a * b / (a + b); }

struct RpSource
{
  uint32_t nominal_uA;
  double min_uA;
  double max_uA;
};

// FUSB302B datasheet current source tolerances.
// The default USB one (80uA +/- 20%) is not supported: the configured Rp is 3A (CONFIG_USB_PD_PULLUP)
const std::vector<RpSource> rpSources = {
        {LMBD_FUSB302_RP_1A5_UA, 166.0, 194.0},
        {LMBD_FUSB302_RP_3A0_UA, 304.0, 356.0},
};

// board Rd, 1% resistor
const std::vector<double> boardRd_ohm = {LMBD_CC_BOARD_RD_OHM * 0.99, LMBD_CC_BOARD_RD_OHM * 1.01};
// sink Rd, Type-C spec allows 5.1k +/- 10%
const std::vector<double> sinkRd_ohm = {4590.0, 5610.0};
// a sink with the same hardware: FUSB302 Rd (4.6k to 5.6k) in parallel with its own board Rd
const std::vector<double> sinkLampRd_ohm = {1.0 / (1.0 / 4600.0 + 1.0 / (LMBD_CC_BOARD_RD_OHM * 0.99)),
                                            1.0 / (1.0 / 5600.0 + 1.0 / (LMBD_CC_BOARD_RD_OHM * 1.01))};
// cable Ra, Type-C spec
const std::vector<double> cableRa_ohm = {800.0, 1200.0};

void expect_level(const CcLevel expected, const std::vector<double>& partner_ohm, const char* what)
{
  for (const auto& rp: rpSources)
  {
    for (const double current_uA: {rp.min_uA, rp.max_uA})
    {
      for (const double board: boardRd_ohm)
      {
        // no partner resistance: board Rd alone
        const std::vector<double> partners = partner_ohm.empty() ? std::vector<double> {0.0} : partner_ohm;
        for (const double partner: partners)
        {
          const double resistance = partner == 0.0 ? board : parallel(board, partner);
          const double cc_mV = current_uA * resistance / 1000.0;
          const CcLevel level = classify(cc_mV, rp.nominal_uA);
          EXPECT_EQ(level, expected) << what << ": Rp " << rp.nominal_uA << "uA (real " << current_uA
                                     << "uA), board Rd " << board << " ohm, partner " << partner << " ohm: CC "
                                     << cc_mV << "mV read as " << to_str(level);
        }
      }
    }
  }
}

} // namespace

TEST(cc_thresholds, nothing_attached_reads_open)
{
  // only the board permanent Rd is on the line: must not be taken for a sink
  expect_level(CcLevel::Open, {}, "nothing attached");
}

TEST(cc_thresholds, sink_reads_rd)
{
  // phone or any sink: its Rd is in parallel with the board Rd
  expect_level(CcLevel::Rd, sinkRd_ohm, "sink attached");
}

TEST(cc_thresholds, sink_lamp_reads_rd)
{
  // another lamp charging from this one: it has the same permanent Rd as us, plus its FUSB302 Rd
  expect_level(CcLevel::Rd, sinkLampRd_ohm, "sink lamp attached");
}

TEST(cc_thresholds, cable_ra_reads_ra)
{
  // e-marked cable on the unused CC line
  expect_level(CcLevel::Ra, cableRa_ohm, "cable Ra");
}

TEST(cc_thresholds, sink_reads_3A_rp_as_1A5)
{
  // CONFIG_USB_PD_PULLUP is 3A: a sink sees our Rp through its Rd in parallel with the board Rd.
  // Type-C sink thresholds: vRd-1.5 is 0.66V to 1.23V. It must never read as default USB (500mA) or 3A (above the
  // 5V 1.5A source PDO)
  for (const double current_uA: {304.0, 356.0})
    for (const double board: boardRd_ohm)
      for (const double sink: sinkRd_ohm)
      {
        const double cc_mV = current_uA * parallel(board, sink) / 1000.0;
        EXPECT_GT(cc_mV, 660.0) << current_uA << "uA, sink Rd " << sink;
        EXPECT_LT(cc_mV, 1230.0) << current_uA << "uA, sink Rd " << sink;
      }
}

TEST(cc_thresholds, partner_in_source_phase_reads_open)
{
  // a dual role partner in its source toggle phase adds its own Rp current to ours, in the board Rd alone:
  // we must not attach to it
  for (const double partner_uA: {64.0, 166.0, 304.0})
    for (const double board: boardRd_ohm)
    {
      const double cc_mV = (304.0 + partner_uA) * board / 1000.0;
      EXPECT_EQ(classify(cc_mV, LMBD_FUSB302_RP_3A0_UA), CcLevel::Open) << "partner Rp " << partner_uA << "uA";
    }
}

TEST(cc_thresholds, mdac_conversion)
{
  // the comparator threshold is (MDAC + 1) * 42mV
  ASSERT_EQ(lmbd_fusb302_mdac_to_mv(0), 42u);
  ASSERT_EQ(lmbd_fusb302_mdac_to_mv(LMBD_FUSB302_MDAC_MAX), 64u * 42u);
  ASSERT_EQ(lmbd_fusb302_mdac_from_mv(42), 0);
  ASSERT_EQ(lmbd_fusb302_mdac_from_mv(0), 0);
  ASSERT_EQ(lmbd_fusb302_mdac_from_mv(100000), LMBD_FUSB302_MDAC_MAX);
  ASSERT_EQ(lmbd_fusb302_mdac_to_mv(lmbd_fusb302_mdac_from_mv(684)), 672u);
}

TEST(cc_thresholds, standard_thresholds_cannot_see_open)
{
  // documents the issue #357: with the standard 1.5A thresholds (vOpen 1.6V, Rd 0.4V), the board Rd alone reads as Rd
  const double cc_mV = LMBD_FUSB302_RP_1A5_UA * LMBD_CC_BOARD_RD_OHM / 1000.0;
  EXPECT_LT(cc_mV, 1600.0);
  EXPECT_GT(cc_mV, 400.0);
}

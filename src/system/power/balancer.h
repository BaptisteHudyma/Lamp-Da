#ifndef POWER_BALANCER_H
#define POWER_BALANCER_H

#include <array>
#include <cstdint>

#include "src/system/utils/constants.h"

namespace balancer {

/**
 * \brief Store the balancer component status
 */
struct Status
{
  /// Cells voltages, in order
  std::array<uint16_t, batteryCount> batteryVoltages_mV;
  /// True if the cell is currently balancing
  std::array<bool, batteryCount> isBalancing;
  /// Voltage of the battery pack
  uint16_t stackVoltage_mV;
  /// Temperature of the component
  uint16_t temperature_degrees;

  /// Last time the measurment was updated
  uint32_t lastMeasurmentUpdate = 0;

  /// True if this structure contains valid data
  bool is_valid() const;
};

/// Return the status of the balancer
Status get_status();

/**
 * \brief Call once on init
 * \return false if the component detect an issue
 */
bool init();

/**
 * \brief call often to refresh measurments
 */
void loop();

/**
 * \brief enable the battery balancing process
 */
void enable_balancing(bool enable);

/**
 * \brief call before the system goes to sleep
 */
void go_to_sleep();

} // namespace balancer

#endif

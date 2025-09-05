#ifndef POWER_BALANCER_H
#define POWER_BALANCER_H

#include <cstdint>

#include "src/system/utils/constants.h"

namespace balancer {

struct Status
{
  // cells voltages, in order
  uint16_t batteryVoltages_mV[batteryCount];
  // true if the cell is currently balancing
  bool isBalancing[batteryCount];
  // voltage of the battery pack
  uint16_t stackVoltage_mV;
  // temperature of the component
  uint16_t temperature_degrees;

  // last time the measurment was updated
  uint32_t lastMeasurmentUpdate = 0;

  bool is_valid() const;
};

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

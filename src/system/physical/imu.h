#ifndef IMU_H
#define IMU_H

#include "LSM6DS3/imu_wrapper.h"

/// Contains the handling of the gyroscope and accelerometer

// IMU is auto activated when used
namespace imu {

/// Called once on program start, initialize the driver
extern void init();

/// Called once on program shutdown, to gracefully close the driver
extern void shutdown();

/**
 * \brief Describe an IMU detection event
 */
enum class EventType
{
  FreeFall,  ///< raised during a free fall event
  BigMotion, ///< raised during a big acceleration
  Step,      ///< raised during step detection
  Tilt,      ///< raised event during orientation flip
};

/// Enable a specific event
bool enable_event_detection(const EventType eventType);
/// Disable a specific event
bool disable_event_detection(const EventType eventType);

/// Read the event bit
bool is_event_detected(const EventType eventType);

/// Enable the interrupt 1 with an event type, wired to the interrupt pin 1 of IMU
extern bool link_event_to_interrupt1(const EventType eventType);
/// Disable the interrupts 1
extern void unlink_interrupt_1();

/// Enable the interrupt 2 with an event type, wired to the interrupt pin 2 of IMU
/// EVENT BigMotion & Step NOT SUPPORTED ON INTERRUPT 2
extern bool link_event_to_interrupt2(const EventType eventType);
/// Disable the interrupts 2
extern void unlink_interrupt_2();

/// Read and reset the interrupt1 bit
bool is_interrupt1_enabled();
/// Read and reset the interrupt2 bit
bool is_interrupt2_enabled();

/// get the filtered IMU readings
Reading get_filtered_reading(const bool resetFilter);

} // namespace imu

#endif

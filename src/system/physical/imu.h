#ifndef IMU_H
#define IMU_H

#include "LSM6DS3/imu_wrapper.h"

/// Contains the handling of the gyroscope and accelerometer

// IMU is auto activated when used
namespace imu {

// close the imu readings
extern void disable();

// disable imu if last use is old
extern void disable_after_non_use();

enum EventType
{
  FreeFall,  // raised during a free fall event
  BigMotion, // raised during a big acceleration
};

// enable event detection
bool enable_detection(const EventType eventType);
// disable event detection (WILL ALSO DISABLE ANY ASSOCIATED INTERRUPT)
void disable_detection(const EventType eventType);

// read the event bit
bool is_event_detected(const EventType eventType);

// enable the interrupt 1 with an event type, wired to the interrupt pin IMU1
extern bool enable_interrupt_1(const EventType eventType);
extern void disable_interrupt_1();

// read and reset the interrupt bit
bool is_interrupt1_enabled();

//
Reading get_filtered_reading(const bool resetFilter);

} // namespace imu

#endif
#ifndef IMU_H
#define IMU_H

#include "LSM6DS3/imu_wrapper.h"

/// Contains the handling of the gyroscope and accelerometer

// IMU is auto activated when used
namespace imu {

extern void init();

extern void shutdown();

enum EventType
{
  FreeFall,  // raised during a free fall event
  BigMotion, // raised during a big acceleration
  Step,      // raised during step detection
  Tilt,      // raised event during orientation flip
};

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
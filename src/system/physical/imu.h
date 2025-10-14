#ifndef IMU_H
#define IMU_H

#include "LSM6DS3/imu_wrapper.h"

/// Contains the handling of the gyroscope and accelerometer

// IMU is auto activated when used
namespace imu {

extern void init();

extern void shutdown();

enum class EventType
{
  FreeFall,  // raised during a free fall event
  BigMotion, // raised during a big acceleration
  Step,      // raised during step detection
  Tilt,      // raised event during orientation flip
};

// enable a specific event
bool enable_event_detection(const EventType eventType);
// disable a specific event
bool disable_event_detection(const EventType eventType);

// read the event bit
bool is_event_detected(const EventType eventType);

// enable the interrupt 1 with an event type, wired to the interrupt pin 1 of IMU
extern bool link_event_to_interrupt1(const EventType eventType);
extern void unlink_interrupt_1();

// enable the interrupt 2 with an event type, wired to the interrupt pin 2 of IMU
// EVENT BigMotion & Step NOT SUPPORTED ON INTERRUPT 2
extern bool link_event_to_interrupt2(const EventType eventType);
extern void unlink_interrupt_2();

// read and reset the interrupt bit
bool is_interrupt1_enabled();

//
Reading get_filtered_reading(const bool resetFilter);

} // namespace imu

#endif

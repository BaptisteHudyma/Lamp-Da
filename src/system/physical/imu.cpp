#include "imu.h"

#include <cstdint>
#include <cstring>

#include "LSM6DS3/imu_wrapper.h"
#include "src/system/utils/print.h"

#include "src/system/platform/time.h"
#include "src/system/platform/gpio.h"

namespace imu {

// instance of the IMU controler
Wrapper imuInstance;

// interrupt 1
DigitalPin interrupt1Pin(DigitalPin::GPIO::Signal_ImuInterrupt1);

void init()
{
  if (not imuInstance.init())
  {
    // TODO: something ?
    lampda_print("fail to start");
  }
  else
  {
    // start each events
    imuInstance.enable_free_fall_detection();
    imuInstance.enable_big_motion_detection();
    imuInstance.enable_step_detection();
    imuInstance.enable_tilt_detection();
  }
}

void shutdown()
{
  // remove callbacks from interrupts
  interrupt1Pin.detach_callbacks();

  // enter deep sleep
  imuInstance.shutdown();
}

bool is_event_detected(const EventType eventType)
{
  switch (eventType)
  {
    case EventType::FreeFall:
      return imuInstance.is_event_detected(Wrapper::InterruptType::FreeFall);

    case EventType::BigMotion:
      return imuInstance.is_event_detected(Wrapper::InterruptType::BigMotion);

    case EventType::Step:
      return imuInstance.is_event_detected(Wrapper::InterruptType::Step);

    case EventType::Tilt:
      return imuInstance.is_event_detected(Wrapper::InterruptType::AngleChange);

    default:
      return false;
  }
}

// interrupt 1 result
bool isInterrupt1Enabled = false;
void interrupt1_callback() { isInterrupt1Enabled = true; }
bool is_interrupt1_enabled()
{
  const bool temp = isInterrupt1Enabled;
  isInterrupt1Enabled = false;
  return temp;
}

// enable the interrupt 1 on event
bool enable_interrupt_1(const EventType eventType)
{
  switch (eventType)
  {
    case EventType::FreeFall:
      if (not imuInstance.enable_interrupt1(Wrapper::InterruptType::FreeFall))
      {
        lampda_print("enable_interrupt_1: enable freefall interrupt failed");
        return false;
      }
      break;
    case EventType::BigMotion:
      if (not imuInstance.enable_interrupt1(Wrapper::InterruptType::BigMotion))
      {
        lampda_print("enable_interrupt_1: enable big motion interrupt failed");
        return false;
      }
      break;
    case EventType::Step:
      if (not imuInstance.enable_interrupt1(Wrapper::InterruptType::Step))
      {
        lampda_print("enable_interrupt_1: enable step interrupt failed");
        return false;
      }
      break;
    case EventType::Tilt:
      if (not imuInstance.enable_interrupt1(Wrapper::InterruptType::AngleChange))
      {
        lampda_print("enable_interrupt_1: enable tilt interrupt failed");
        return false;
      }
      break;
    default:
      // do not enable interrupt
      return false;
  }

  // prevent shutdown when no events
  interrupt1Pin.set_pin_mode(DigitalPin::Mode::kInput);
  interrupt1Pin.detach_callbacks();
  interrupt1Pin.attach_callback(interrupt1_callback, DigitalPin::Interrupt::kChange);
  return true;
}

void disable_interrupt_1() { imuInstance.disable_interrupt1(); }

Reading get_filtered_reading(const bool resetFilter)
{
  static Reading filtered;

  const Reading& read = imuInstance.get_reading();
  if (resetFilter)
  {
    filtered = read;
  }
  else
  {
    // simple linear filter: average on the last 10 values
    filtered.accel.x += 0.1 * (read.accel.x - filtered.accel.x);
    filtered.accel.y += 0.1 * (read.accel.y - filtered.accel.y);
    filtered.accel.z += 0.1 * (read.accel.z - filtered.accel.z);

    filtered.gyro.x += 0.1 * (read.gyro.x - filtered.gyro.x);
    filtered.gyro.y += 0.1 * (read.gyro.y - filtered.gyro.y);
    filtered.gyro.z += 0.1 * (read.gyro.z - filtered.gyro.z);
  }

  return filtered;
}

} // namespace imu
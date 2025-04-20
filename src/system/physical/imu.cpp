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

static uint32_t lastIMUFunctionCall = 0;
bool isStarted = false;

// when set to true, the imu will not autostop
bool lockAutoStop = false;

// interrupt 1
DigitalPin interrupt1Pin(DigitalPin::GPIO::Signal_ImuInterrupt1);
bool isInterruptEnabled = false;

void enable()
{
  lastIMUFunctionCall = time_ms();
  if (isStarted)
    return;

  if (not imuInstance.init())
  {
    // TODO: something ?
    lampda_print("fail to start");
  }
  else
    isStarted = true;
}

void disable()
{
  if (!isStarted)
  {
    return;
  }

  // remove callbacks from interrupts
  interrupt1Pin.detach_callbacks();

  // enter deep sleep
  imuInstance.shutdown();

  isStarted = false;
}

void disable_after_non_use()
{
  if (!isStarted)
    return;
  if (lockAutoStop)
    return;

  // started, allow to auto stop, and 1 second without events
  if (time_ms() - lastIMUFunctionCall > 1000.0)
  {
    // disable imu if last reading is old
    disable();
    lampda_print("imu stop: non use");
  }
}

void interrupt1_callback() { isInterruptEnabled = true; }

bool enable_detection(const EventType eventType)
{
  enable();
  if (not isStarted)
  {
    lampda_print("enable_detection: failed to start imu");
    return false;
  }

  switch (eventType)
  {
    case FreeFall:
      if (not imuInstance.enable_detection(Wrapper::InterruptType::Fall))
      {
        lampda_print("enable_detection: enable freefall interrupt failed");
        return false;
      }
      break;
    default:
      // do not enable interrupt
      return false;
  }
  return true;
}

void disable_detection(const EventType eventType)
{
  switch (eventType)
  {
    case FreeFall:
      imuInstance.disable_detection(Wrapper::InterruptType::Fall);
      break;
    default:
      break;
  }
}

bool is_event_detected(const EventType eventType)
{
  switch (eventType)
  {
    case FreeFall:
      return imuInstance.is_event_detected(Wrapper::InterruptType::Fall);

    default:
      return false;
  }
}

bool enable_interrupt_1(const EventType eventType)
{
  enable();
  if (not isStarted)
  {
    lampda_print("enable_interrupt_1: failed to start imu");
    return false;
  }

  switch (eventType)
  {
    case FreeFall:
      if (not imuInstance.enable_interrupt1(Wrapper::InterruptType::Fall))
      {
        lampda_print("enable_interrupt_1: enable freefall interrupt failed");
        return false;
      }
      break;
    default:
      // do not enable interrupt
      return false;
  }

  // prevent shutdown when no events
  lockAutoStop = true;
  interrupt1Pin.set_pin_mode(DigitalPin::Mode::kInput);
  interrupt1Pin.detach_callbacks();
  interrupt1Pin.attach_callback(interrupt1_callback, DigitalPin::Interrupt::kChange);
  return true;
}

void disable_interrupt_1()
{
  imuInstance.enable_interrupt1(Wrapper::InterruptType::None);
  // allow auto shutdown
  lockAutoStop = false;
}

bool is_interrupt1_enabled()
{
  const bool temp = isInterruptEnabled;
  isInterruptEnabled = false;
  return temp;

  // return imuInstance.is_interrupt_raised(Wrapper::InterruptType::Fall);
}

Reading get_filtered_reading(const bool resetFilter)
{
  static Reading filtered;
  enable();
  if (not isStarted)
  {
    lampda_print("get_filtered_reading: failed to start imu");
    return filtered;
  }

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
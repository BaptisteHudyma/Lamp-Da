#include "imu.h"

#include <cstring>

#include "LSM6DS3/imu_wrapper.h"
#include "src/system/utils/constants.h"
#include "src/system/utils/vector_math.h"

#include "src/system/platform/time.h"
#include "src/system/platform/gpio.h"
#include "src/system/platform/print.h"

namespace imu {

// instance of the IMU controler
Wrapper imuInstance;

// interrupt 1
DigitalPin interrupt1Pin(DigitalPin::GPIO::Signal_ImuInterrupt1);
DigitalPin interrupt2Pin(DigitalPin::GPIO::Signal_ImuInterrupt2);

bool isInitialized = false;

// transform imu coordinates to board coordinates
const TransformationMatrix imuToBoardTransformation(
        vec3d(imuToCircuitRotationX_rad, imuToCircuitRotationY_rad, imuToCircuitRotationZ_rad),
        vec3d(imuToCircuitPositionX_m, imuToCircuitPositionY_m, imuToCircuitPositionZ_m));

// transform board coordinates to first pixel
const TransformationMatrix boardToFirstPixelTransformation(vec3d(circuitToLedZeroRotationX_degrees* c_degreesToRadians,
                                                                 circuitToLedZeroRotationY_degrees* c_degreesToRadians,
                                                                 circuitToLedZeroRotationZ_degrees* c_degreesToRadians),
                                                           vec3d(0, 0, 0));

void init()
{
  if (not imuInstance.init())
  {
    // TODO: something ?
    lampda_print("IMU failed to start");
    isInitialized = false;
  }
  else
  {
    isInitialized = true;
  }
}

void shutdown()
{
  // remove callbacks from interrupts
  interrupt1Pin.detach_callbacks();

  // enter deep sleep
  imuInstance.shutdown();
}

bool enable_event_detection(const EventType eventType)
{
  if (not isInitialized)
    return false;

  switch (eventType)
  {
    case EventType::FreeFall:
      return imuInstance.enable_free_fall_detection();

    case EventType::BigMotion:
      return imuInstance.enable_big_motion_detection();

    case EventType::Step:
      return imuInstance.enable_step_detection();

    case EventType::Tilt:
      return imuInstance.enable_tilt_detection();

    default:
      return false;
  }
}

bool disable_event_detection(const EventType eventType)
{
  if (not isInitialized)
    return false;

  switch (eventType)
  {
    case EventType::FreeFall:
      return imuInstance.disable_free_fall_detection();

    case EventType::BigMotion:
      return imuInstance.disable_big_motion_detection();

    case EventType::Step:
      return imuInstance.disable_step_detection();

    case EventType::Tilt:
      return imuInstance.disable_tilt_detection();

    default:
      return false;
  }
}

bool is_event_detected(const EventType eventType)
{
  if (not isInitialized)
    return false;

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

// interrupt 2 result
bool isInterrupt2Enabled = false;
void interrupt2_callback() { isInterrupt2Enabled = true; }
bool is_interrupt2_enabled()
{
  const bool temp = isInterrupt2Enabled;
  isInterrupt2Enabled = false;
  return temp;
}

// enable the interrupt 1 on event
bool link_event_to_interrupt1(const EventType eventType)
{
  if (not isInitialized)
    return false;

  switch (eventType)
  {
    case EventType::FreeFall:
      if (not imuInstance.enable_interrupt1(Wrapper::InterruptType::FreeFall))
      {
        lampda_print("link_event_to_interrupt1: enable freefall interrupt failed");
        return false;
      }
      break;
    case EventType::BigMotion:
      if (not imuInstance.enable_interrupt1(Wrapper::InterruptType::BigMotion))
      {
        lampda_print("link_event_to_interrupt1: enable big motion interrupt failed");
        return false;
      }
      break;
    case EventType::Step:
      if (not imuInstance.enable_interrupt1(Wrapper::InterruptType::Step))
      {
        lampda_print("link_event_to_interrupt1: enable step interrupt failed");
        return false;
      }
      break;
    case EventType::Tilt:
      if (not imuInstance.enable_interrupt1(Wrapper::InterruptType::AngleChange))
      {
        lampda_print("link_event_to_interrupt1: enable tilt interrupt failed");
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

void unlink_interrupt_1() { imuInstance.disable_interrupt1(); }

// enable the interrupt 2 on event
bool link_event_to_interrupt2(const EventType eventType)
{
  if (not isInitialized)
    return false;

  switch (eventType)
  {
    case EventType::FreeFall:
      if (not imuInstance.enable_interrupt2(Wrapper::InterruptType::FreeFall))
      {
        lampda_print("link_event_to_interrupt2: enable freefall interrupt failed");
        return false;
      }
      break;
    case EventType::BigMotion:
      {
        lampda_print("link_event_to_interrupt2: big motion interrupt not supported for pin2");
        return false;
      }
    case EventType::Step:

      {
        lampda_print("link_event_to_interrupt2: step interrupt not supported for pin2");
        return false;
      }
    case EventType::Tilt:
      if (not imuInstance.enable_interrupt2(Wrapper::InterruptType::AngleChange))
      {
        lampda_print("link_event_to_interrupt2: enable tilt interrupt failed");
        return false;
      }
      break;
    default:
      // do not enable interrupt
      return false;
  }

  // prevent shutdown when no events
  interrupt2Pin.set_pin_mode(DigitalPin::Mode::kInput);
  interrupt2Pin.detach_callbacks();
  interrupt2Pin.attach_callback(interrupt2_callback, DigitalPin::Interrupt::kChange);
  return true;
}

void unlink_interrupt_2() { imuInstance.disable_interrupt2(); }

Reading get_filtered_reading(const bool resetFilter)
{
  static Reading filtered;

  constexpr float oneG = 9.80665f;

  const Reading& read = imuInstance.get_reading();
  if (resetFilter)
  {
    filtered = read;
    filtered.accel.x *= oneG;
    filtered.accel.y *= oneG;
    filtered.accel.z *= oneG;
  }
  else
  {
    // simple linear filter: average on the last 10 values
    filtered.accel.x += 0.1f * (read.accel.x * oneG - filtered.accel.x);
    filtered.accel.y += 0.1f * (read.accel.y * oneG - filtered.accel.y);
    filtered.accel.z += 0.1f * (read.accel.z * oneG - filtered.accel.z);

    filtered.gyro.x += 0.1f * (read.gyro.x - filtered.gyro.x);
    filtered.gyro.y += 0.1f * (read.gyro.y - filtered.gyro.y);
    filtered.gyro.z += 0.1f * (read.gyro.z - filtered.gyro.z);
  }

  // transform to lamp body space
  // this new object is necessary or the computation returns garbage. TODO: WHY
  Reading lampSpaceVector;
  lampSpaceVector.accel = boardToFirstPixelTransformation.transform(imuToBoardTransformation.transform(filtered.accel));
  lampSpaceVector.gyro = boardToFirstPixelTransformation.transform(imuToBoardTransformation.transform(filtered.gyro));
  // inverse z axis
  lampSpaceVector.accel.x = -lampSpaceVector.accel.x;
  lampSpaceVector.accel.y = -lampSpaceVector.accel.y;
  lampSpaceVector.accel.z = -lampSpaceVector.accel.z;
  return lampSpaceVector;
}

} // namespace imu

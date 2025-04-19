#include "IMU.h"

#include <cstdint>
#include <cstring>

#include "LSM6DS3/LSM6DS3.h"
#include "src/system/utils/print.h"

#include "src/system/platform/time.h"
#include "src/system/platform/gpio.h"
#include "src/system/platform/i2c.h"

namespace imu {

// Create a instance of class LSM6DS3
LSM6DS3 IMU(I2C_MODE, imuI2cAddress); // I2C device address

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
  {
    return;
  }

  delay_ms(5); // voltage stabilization

  if (IMU.begin() != IMU_SUCCESS)
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

  // TODO: enter deep sleep

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
    // disable microphone if last reading is old
    disable();
    lampda_print("imu stop: non use");
  }
}

void interrupt1_callback() { isInterruptEnabled = true; }

bool enable_interrupt_1(EventType eventType)
{
  enable();
  if (not isStarted)
  {
    return false;
  }

  switch (eventType)
  {
    case FreeFall:
      if (not IMU.enable_interrupt1(LSM6DS3::InterruptType::Fall))
      {
        lampda_print("enable fail");
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
  interrupt1Pin.attach_callback(interrupt1_callback, DigitalPin::Interrupt::kRisingEdge);
  return true;
}

void disable_interrupt_1()
{
  IMU.enable_interrupt1(LSM6DS3::InterruptType::None);
  // allow auto shutdown
  lockAutoStop = false;
}

bool is_interrupt1_enabled()
{
  const bool temp = isInterruptEnabled;
  isInterruptEnabled = false;
  return temp;
}

Reading get_reading()
{
  enable();

  Reading reads;
  // coordinate change to the lamp body
  reads.accel.x = IMU.readFloatAccelX();
  reads.accel.y = IMU.readFloatAccelY();
  reads.accel.z = IMU.readFloatAccelZ();

  reads.gyro.x = IMU.readFloatGyroX();
  reads.gyro.y = IMU.readFloatGyroY();
  reads.gyro.z = IMU.readFloatGyroZ();
  return reads;
}

Reading get_filtered_reading(const bool resetFilter)
{
  static Reading filtered;

  const Reading& read = get_reading();
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
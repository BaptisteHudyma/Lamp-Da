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

void enable()
{
  lastIMUFunctionCall = time_ms();
  if (isStarted)
  {
    return;
  }

  delay_ms(5); // voltage stabilization

  if (IMU.begin() != 0)
  {
    // TODO: something ?
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

  // TODO: enter deep sleep

  isStarted = false;
}

void disable_after_non_use()
{
  if (isStarted and (time_ms() - lastIMUFunctionCall > 1000.0))
  {
    // disable microphone if last reading is old
    disable();
  }
}

struct vec3d
{
  float x;
  float y;
  float z;
};

struct Accelerometer : public vec3d
{
};
struct Gyroscope : public vec3d
{
};

struct Reading
{
  Accelerometer accel;
  Gyroscope gyro;
};

Reading get_reading()
{
  enable();

  Reading reads;
  // coordinate change to the lamp body
  reads.accel.x = IMU.readFloatAccelX();
  reads.accel.y = IMU.readFloatAccelY();
  reads.accel.z = IMU.readFloatAccelZ();

  // use this to debug the axes
#if 1
  lampda_print("%f, %f, %f", reads.accel.x, reads.accel.y, reads.accel.z);
#endif

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
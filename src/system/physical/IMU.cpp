#include "IMU.h"

#include <LSM6DS3.h>

#include <cstdint>
#include <cstring>

#include "Arduino.h"

namespace imu {

// Create a instance of class LSM6DS3
LSM6DS3 IMU(I2C_MODE, 0x6A);  // I2C device address 0x6A

bool isStarted = false;
void enable() {
  if (isStarted) {
    return;
  }

  pinMode(PIN_LSM6DS3TR_C_POWER, OUTPUT);
  digitalWrite(PIN_LSM6DS3TR_C_POWER, HIGH);

  if (IMU.begin() != 0) {
  }

  isStarted = true;
}

void disable() {
  if (!isStarted) {
    return;
  }

  digitalWrite(PIN_LSM6DS3TR_C_POWER, LOW);
  isStarted = false;
}

struct vec3d {
  float x;
  float y;
  float z;
};

struct Accelerometer : public vec3d {};
struct Gyroscope : public vec3d {};

struct Reading {
  Accelerometer accel;
  Gyroscope gyro;
};

Reading get_reading() {
  if (!isStarted) enable();

  Reading reads;
  // coordinate change to the lamp body
  // it would be better with transformations matrices but hey, microcontrolers!
  reads.accel.x = -IMU.readFloatAccelZ();
  reads.accel.y = -IMU.readFloatAccelY();
  reads.accel.z = IMU.readFloatAccelX();

  // use this to debug the axes
#if 1
  Serial.print(reads.accel.x);
  Serial.print(",");
  Serial.print(reads.accel.y);
  Serial.print(",");
  Serial.print(reads.accel.z);
  Serial.println("");
#endif

  reads.gyro.x = -IMU.readFloatGyroZ();
  reads.gyro.y = -IMU.readFloatGyroY();
  reads.gyro.z = IMU.readFloatGyroX();
  return reads;
}

Reading get_filtered_reading(const bool resetFilter) {
  static Reading filtered;

  const Reading& read = get_reading();
  if (resetFilter) {
    filtered = read;
  } else {
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

constexpr uint16_t N_GRAINS = 128;  // Number of grains
struct Grain {
  int16_t x, y;    // Position
  int16_t vx, vy;  // Velocity
  uint16_t pos;
} grain[N_GRAINS];

}  // namespace imu
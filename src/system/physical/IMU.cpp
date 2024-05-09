#include "IMU.h"

#include <cstdint>
#include <cstring>

#include "Arduino.h"
#include "LSM6DS3/LSM6DS3.h"

namespace imu {

// Create a instance of class LSM6DS3
LSM6DS3 IMU(I2C_MODE, 0x6A);  // I2C device address 0x6A

static uint32_t lastIMUFunctionCall = 0;
bool isStarted = false;

void enable() {
  lastIMUFunctionCall = millis();
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

void disable_after_non_use() {
  if (isStarted and (millis() - lastIMUFunctionCall > 1000.0)) {
    // disable microphone if last reading is old
    disable();
  }
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
  enable();

  Reading reads;
  // coordinate change to the lamp body
  reads.accel.x = IMU.readFloatAccelX();
  reads.accel.y = IMU.readFloatAccelY();
  reads.accel.z = IMU.readFloatAccelZ();

  // use this to debug the axes
#if 0
  Serial.print(reads.accel.x);
  Serial.print(",");
  Serial.print(reads.accel.y);
  Serial.print(",");
  Serial.print(reads.accel.z);
  Serial.println("");
#endif

  reads.gyro.x = IMU.readFloatGyroX();
  reads.gyro.y = IMU.readFloatGyroY();
  reads.gyro.z = IMU.readFloatGyroZ();
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

void gravity_fluid(const uint8_t fade, const Color& color, LedStrip& strip,
                   const bool isReset) {
  constexpr uint16_t WIDTH = stripXCoordinates;
  constexpr uint16_t HEIGHT = stripYCoordinates;

  const float scaling = 100.0;

  constexpr uint16_t MAX_X =
      (WIDTH * 256 - 1);  // Maximum X coordinate in grain space
  constexpr uint16_t MAX_Y =
      (HEIGHT * 256 - 1);  // Maximum Y coordinate  in grain space

  static uint16_t* img = strip._buffer16b;

  static bool isInit = false;
  if (!isInit or isReset) {
    memset(strip._buffer16b, 0, sizeof(strip._buffer16b));

    uint16_t j = 0;
    for (uint16_t i = 0; i < N_GRAINS; i++) {  // For each sand grain...
      do {
        grain[i].x = random(WIDTH * 256);   // Assign random position within
        grain[i].y = random(HEIGHT * 256);  // the 'grain' coordinate space
        // Check if corresponding pixel position is already occupied...
        for (j = 0; (j < i) && (((grain[i].x / 256) != (grain[j].x / 256)) ||
                                ((grain[i].y / 256) != (grain[j].y / 256)));
             j++)
          ;
      } while (j < i);  // Keep retrying until a clear spot is found

      img[(grain[i].y / 256) * WIDTH + (grain[i].x / 256)] = 255;  // Mark it

      grain[i].pos = (grain[i].y / 256) * WIDTH + (grain[i].x / 256);
      grain[i].vx = grain[i].vy = 0;  // Initial velocity is zero
    }

    // init set to true, must reset to go back to the start
    isInit = true;
    return;
  }

  const auto& reading = get_reading();

  constexpr float accelerometerScale = 9.81 * 256;
  int16_t ax = reading.accel.y * scaling * accelerometerScale /
               256;  // Transform accelerometer axes
  int16_t ay = reading.accel.x * scaling * accelerometerScale /
               256;  // to grain coordinate space
  int16_t az = reading.accel.z * scaling * accelerometerScale / 256;

  int16_t az2 = az * 2 + 1;  // Range of random motion to add back in

  // ...and apply 2D accel vector to grain velocities...
  int32_t v2;  // Velocity squared
  float v;     // Absolute velocity
  for (int i = 0; i < N_GRAINS; i++) {
    grain[i].vx += ax;                // A little randomness makes
    grain[i].vy += ay + random(az2);  // tall stacks topple better!
    // Terminal velocity (in any direction) is 256 units -- equal to
    // 1 pixel -- which keeps moving grains from passing through each other
    // and other such mayhem.  Though it takes some extra math, velocity is
    // clipped as a 2D vector (not separately-limited X & Y) so that
    // diagonal movement isn't faster
    v2 =
        (int32_t)grain[i].vx * grain[i].vx + (int32_t)grain[i].vy * grain[i].vy;
    if (v2 > 65536) {       // If v^2 > 65536, then v > 256
      v = sqrt((float)v2);  // Velocity vector magnitude
      grain[i].vx = (int)(256.0 * (float)grain[i].vx / v);  // Maintain heading
      grain[i].vy = (int)(256.0 * (float)grain[i].vy / v);  // Limit magnitude
    }
  }

  // ...then update position of each grain, one at a time, checking for
  // collisions and having them react.  This really seems like it shouldn't
  // work, as only one grain is considered at a time while the rest are
  // regarded as stationary.  Yet this naive algorithm, taking many not-
  // technically-quite-correct steps, and repeated quickly enough,
  // visually integrates into something that somewhat resembles physics.
  // (I'd initially tried implementing this as a bunch of concurrent and
  // "realistic" elastic collisions among circular grains, but the
  // calculations and volument of code quickly got out of hand for both
  // the tiny 8-bit AVR microcontroller and my tiny dinosaur brain.)

  uint16_t i, bytes, oldidx, newidx, delta;
  int16_t newx, newy;

  for (i = 0; i < N_GRAINS; i++) {
    newx = grain[i].x + grain[i].vx;  // New position in grain space
    newy = grain[i].y + grain[i].vy;
    if (newx > MAX_X) {   // If grain would go out of bounds
      newx = MAX_X;       // keep it inside, and
      grain[i].vx /= -2;  // give a slight bounce off the wall
    } else if (newx < 0) {
      newx = 0;
      grain[i].vx /= -2;
    }
    // bounce of the walls
    if (newy > MAX_Y) {
      newy = MAX_Y;
      grain[i].vy /= -2;
    } else if (newy < 0) {
      newy = 0;
      grain[i].vy /= -2;
    }

    oldidx = (grain[i].y / 256) * WIDTH + (grain[i].x / 256);  // Prior pixel #
    newidx = (newy / 256) * WIDTH + (newx / 256);              // New pixel #
    if ((oldidx != newidx) &&        // If grain is moving to a new pixel...
        img[newidx]) {               // but if that pixel is already occupied...
      delta = abs(newidx - oldidx);  // What direction when blocked?
      if (delta == 1) {              // 1 pixel left or right)
        newx = grain[i].x;           // Cancel X motion
        grain[i].vx /= -2;           // and bounce X velocity (Y is OK)
        newidx = oldidx;             // No pixel change
      } else if (delta == WIDTH) {   // 1 pixel up or down
        newy = grain[i].y;           // Cancel Y motion
        grain[i].vy /= -2;           // and bounce Y velocity (X is OK)
        newidx = oldidx;             // No pixel change
      } else {                       // Diagonal intersection is more tricky...
        // Try skidding along just one axis of motion if possible (start w/
        // faster axis).  Because we've already established that diagonal
        // (both-axis) motion is occurring, moving on either axis alone WILL
        // change the pixel index, no need to check that again.
        if ((abs(grain[i].vx) - abs(grain[i].vy)) >= 0) {  // X axis is faster
          newidx = (grain[i].y / 256) * WIDTH + (newx / 256);
          if (!img[newidx]) {   // That pixel's free!  Take it!  But...
            newy = grain[i].y;  // Cancel Y motion
            grain[i].vy /= -2;  // and bounce Y velocity
          } else {              // X pixel is taken, so try Y...
            newidx = (newy / 256) * WIDTH + (grain[i].x / 256);
            if (!img[newidx]) {   // Pixel is free, take it, but first...
              newx = grain[i].x;  // Cancel X motion
              grain[i].vx /= -2;  // and bounce X velocity
            } else {              // Both spots are occupied
              newx = grain[i].x;  // Cancel X & Y motion
              newy = grain[i].y;
              grain[i].vx /= -2;  // Bounce X & Y velocity
              grain[i].vy /= -2;
              newidx = oldidx;  // Not moving
            }
          }
        } else {  // Y axis is faster, start there
          newidx = (newy / 256) * WIDTH + (grain[i].x / 256);
          if (!img[newidx]) {   // Pixel's free!  Take it!  But...
            newx = grain[i].x;  // Cancel X motion
            grain[i].vy /= -2;  // and bounce X velocity
          } else {              // Y pixel is taken, so try X...
            newidx = (grain[i].y / 256) * WIDTH + (newx / 256);
            if (!img[newidx]) {   // Pixel is free, take it, but first...
              newy = grain[i].y;  // Cancel Y motion
              grain[i].vy /= -2;  // and bounce Y velocity
            } else {              // Both spots are occupied
              newx = grain[i].x;  // Cancel X & Y motion
              newy = grain[i].y;
              grain[i].vx /= -2;  // Bounce X & Y velocity
              grain[i].vy /= -2;
              newidx = oldidx;  // Not moving
            }
          }
        }
      }
    }
    grain[i].x = newx;  // Update grain position
    grain[i].y = newy;
    img[oldidx] = 0;    // Clear old spot (might be same as new, that's OK)
    img[newidx] = 255;  // Set new spot
    grain[i].pos = newidx;
  }

  strip.fadeToBlackBy(fade);
  for (i = 0; i < N_GRAINS; i++) {
    int yPos = grain[i].pos / WIDTH;
    int xPos = grain[i].pos % WIDTH;

    const auto& stripCoord = to_strip(xPos, yPos);
    strip.setPixelColor(stripCoord, color.get_color(i, N_GRAINS));
  }
}

}  // namespace imu
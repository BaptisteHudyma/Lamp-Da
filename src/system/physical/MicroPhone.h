#ifndef MICRO_PHONE_H
#define MICRO_PHONE_H

#include <stdint.h>
namespace microphone {

// decibel level for a silent room
constexpr float silenceLevelDb = -57.0;
constexpr float highLevelDb =
    80.0;  // microphone is not good enough at after this
constexpr uint8_t numberOfFFtChanels = 16;

// start the microphone readings
extern void enable();
// close the microphone readings
extern void disable();

// disable microphone if last use is old
extern void disable_after_non_use();

/**
 * \return the average sound level in decibels
 */
extern float get_sound_level_Db();

struct SoundStruct {
  bool isValid = false;

  float fftMajorPeakFrequency_Hz = 0.0;
  float strongestPeakMagnitude = 0.0;
  uint8_t fft[numberOfFFtChanels];
};

/**
 * \brief compute the fast fourrier transform of the most recent sound sample
 * \return The frequency analysis object
 */
extern SoundStruct get_fft();

}  // namespace microphone

#endif
#ifndef PHYSICAL_SOUND_H
#define PHYSICAL_SOUND_H

#include "src/system/platform/pdm_handle.h"

namespace microphone {

// decibel level for a silent room
constexpr float silenceLevelDb = -10.0;
// microphone is not good enough at after this
constexpr float highLevelDb = 80.0;

struct SoundStruct
{
  /**
   * raw data
   */

  bool isDataValid = false;
  static constexpr size_t SAMPLE_SIZE = PdmData::SAMPLE_SIZE;
  std::array<int16_t, SAMPLE_SIZE> data;

  float sound_level_Db = 0.0f;

  /**
   * FFT
   */

  bool isFFTValid = false;
  // make this number close to the lamp max x coordinates
  static constexpr uint8_t numberOfFFtChanels = 25;

  // FFT results
  float fftMajorPeakFrequency_Hz = 0.0;
  float strongestPeakMagnitude = 0.0;
  std::array<uint8_t, numberOfFFtChanels> fft;
};

bool enable();
void disable();

// disable microphone if last use is old
void disable_after_non_use();

/**
 * \brief Compute and process sound data
 * \return the last sound data
 */
SoundStruct get_sound_characteristics();

} // namespace microphone

#endif

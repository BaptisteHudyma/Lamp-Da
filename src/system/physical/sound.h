#ifndef PHYSICAL_SOUND_H
#define PHYSICAL_SOUND_H

#include "src/system/platform/pdm_handle.h"
#include <cstdint>

namespace microphone {

// decibel level for a silent room
constexpr float silenceLevelDb = 30.0;
// microphone is not good enough at after this
constexpr float highLevelDb = 120.0;

// desired output of the auto gain (0-1)
constexpr float desiredoutput = 0.5f;
constexpr int16_t gainedSignalTarget = INT16_MAX * desiredoutput;

struct SoundStruct
{
  /**
   * raw data
   */

  bool isDataValid = false;
  static constexpr auto SAMPLE_SIZE = PdmData::SAMPLE_SIZE;
  std::array<int16_t, SAMPLE_SIZE> data;

  // data with auto gain enabled
  std::array<int16_t, SAMPLE_SIZE> rectifiedData;

  float sound_level_Db = 0.0f;

  /**
   * FFT
   */

  bool isFFTValid = false;
  // make this number close to the lamp max x coordinates
  static constexpr uint8_t numberOfFFtChanels = 24;

  static float get_fft_resolution_Hz();

  // FFT results
  std::array<float, SAMPLE_SIZE / 2> fft_raw;
  std::array<float, numberOfFFtChanels> fft_log;
  std::array<float, numberOfFFtChanels> fft_log_end_frequencies;
};

bool enable();
void disable();

// disable microphone if last use is old
void disable_after_non_use();

/**
 * \brief Compute and process sound data
 * \return the last sound data
 */
SoundStruct& get_sound_characteristics();

} // namespace microphone

#endif

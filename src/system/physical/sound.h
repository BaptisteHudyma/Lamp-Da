/*! \file sound.h
    \brief Interface for the physical components of the microphone.
*/

#ifndef PHYSICAL_SOUND_H
#define PHYSICAL_SOUND_H

#include "src/system/platform/pdm_handle.h"
#include <cstdint>
#include "src/system/utils/fft.h"

/// Handle the microphone physical interface
namespace microphone {

/// Decibel level for a silent room
constexpr float silenceLevelDb = 30.0;
/// Microphone is not good enough at after this
constexpr float highLevelDb = 80.0;

/// Desired output of the auto gain (0-1)
constexpr float desiredoutput = 0.5f;
/// Desired output of the auto gain (0-INT16_MAX)
constexpr int16_t gainedSignalTarget = INT16_MAX * desiredoutput;

/**
 * \brief Handle the analysis of a sound sample.
 * This structure handled the FastFourrier analysis of a sound sample, sound strenght, sound most active frequency, ...
 */
struct SoundStruct
{
  /*
   * raw data
   */

  /// flag that indicate sound data validity
  bool isDataValid = false;

  /// Size fo the audio sample
  static constexpr auto SAMPLE_SIZE = PdmData::SAMPLE_SIZE;

  /// raw audio data
  std::array<int16_t, SAMPLE_SIZE> data;
  /// audio data with auto gain enabled
  std::array<int16_t, SAMPLE_SIZE> rectifiedData;

  /// Sound level of this sample, in Decibels A
  float sound_level_Db = 0.0f;
  /// Maximum detected amplitude of this sample
  float maxAmplitude = 0.0f;
  /// Maximum detected amplitude frequency of this sample, in Hertz
  float maxAmplitudeFrequency = 0.0f;

  /*
   * FFT
   */

  /// validity flag for the FFT results
  bool isFFTValid = false;

  /// Define the number of FFt bins to use. You should set this number close to the lamp max X coordinates
  static constexpr uint8_t numberOfFFtChanels = 24;

  /// Return the FFT resolution of a single FFT bin in Hertz
  static constexpr float get_fft_resolution_Hz() { return SAMPLE_RATE / static_cast<float>(SAMPLE_SIZE); }

  // FFT results

  /// Results of the FFT process, in raw Hertz bins
  std::array<float, SAMPLE_SIZE / 2> fft_raw;
  /// Results of the FFT process, in scaled logarithmic bins. This is closer to the sound sensitivity of the Human ear
  std::array<float, numberOfFFtChanels> fft_log;
  /// Results of the FFT process, in maximum frequency for every bin
  std::array<float, numberOfFFtChanels> fft_log_end_frequencies;
};

/**
 * \brief Start the microphone data analysis, if not already started
 * \return True if the process started
 */
bool enable();

/**
 * \brief Disable the microphone data analysis, if its running
 */
void disable();

/**
 * \brief disable microphone if last used a target time ago
 */
void disable_after_non_use();

/**
 * \brief Compute and process sound data
 * \return the last sound data
 */
SoundStruct& get_sound_characteristics();

} // namespace microphone

#endif

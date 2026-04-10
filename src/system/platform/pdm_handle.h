#ifndef PLATFORM_PDM_HANDLE_H
#define PLATFORM_PDM_HANDLE_H

#include <array>
#include <stdint.h>
#include <cstddef>

namespace microphone {

/// CALIBRATED value of a gain baseline
static constexpr int16_t gainUpdateBaseline = 80;

/**
 * \brief Store the audio data obtained from the microphone
 */
struct PdmData
{
  /// Size of the sound sample
  static constexpr uint16_t SAMPLE_SIZE = 512;
  /// raw audio data buffer
  std::array<int16_t, SAMPLE_SIZE> data;
  /// duration of the sample in microseconds
  uint64_t sampleDuration_us;
  /// time at the start of the sample in microseconds
  uint64_t sampleTime_us;

  /// number of sound sample in array. Will be <= SAMPLE_SIZE
  uint16_t sampleRead = 0;
  /// if this is false, all the data are garbage
  bool is_valid() const { return sampleRead > 0; }
};

namespace _private {

PdmData get();

// start the microphone readings
bool start();
// close the microphone readings
void stop();

} // namespace _private

} // namespace microphone

#endif

#ifndef PLATFORM_PDM_HANDLE_H
#define PLATFORM_PDM_HANDLE_H

#include <array>
#include <stdint.h>
#include <cstddef>

namespace microphone {

struct PdmData
{
  static constexpr size_t SAMPLE_SIZE = 512; // same has samplesFFT;
  std::array<int16_t, SAMPLE_SIZE> data;

  uint64_t sampleDuration_us;
  uint64_t sampleTime_us;

  // number of sound sample in array
  uint32_t sampleRead = 0;

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

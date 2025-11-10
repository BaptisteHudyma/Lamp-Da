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

  uint32_t sampleDuration_us;
  uint32_t sampleTime_us;

  // number of sound sample in array
  uint32_t sampleRead = 0;

  bool is_valid() const { return sampleRead > 0; } // and (time_us() - sampleTime_us) < 200; }
};

struct SoundStruct
{
  // make this number close to the lamp max x coordinates
  static constexpr uint8_t numberOfFFtChanels = 25;

  bool isValid = false;

  float fftMajorPeakFrequency_Hz = 0.0;
  float strongestPeakMagnitude = 0.0;
  std::array<uint8_t, numberOfFFtChanels> fft;
};

namespace _private {

PdmData get();

// start the microphone readings
bool start();
// close the microphone readings
void stop();

/**
 * \brief compute the fast fourrier transform of the most recent sound sample
 * \return The frequency analysis object
 */
SoundStruct process_fft(const PdmData& data);

} // namespace _private

} // namespace microphone

#endif

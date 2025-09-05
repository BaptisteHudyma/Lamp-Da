#ifndef PDM_HANDLE_CPP
#define PDM_HANDLE_CPP

#include "pdm_handle.h"
#include "src/system/platform/time.h"

#include <PDM.h>
#include "fft.h"

namespace microphone {

FftAnalyzer<PdmData::SAMPLE_SIZE, SoundStruct::numberOfFFtChanels> fftAnalyzer;

PdmData lastData;

// callback every time the microphone reads data
void on_PDM_data()
{
  lastData.sampleRead = 0;
  const uint32_t newTime = time_us();

  lastData.sampleDuration_us = newTime - lastData.sampleTime_us;
  lastData.sampleTime_us = newTime;
  // query the number of bytes available
  const uint16_t bytesAvailable = min(PDM.available(), PdmData::SAMPLE_SIZE * 2);

  // read into the sample buffer
  PDM.read((char*)&lastData.data[0], bytesAvailable);

  // number of samples read
  lastData.sampleRead = bytesAvailable / 2;
}

namespace _private {

PdmData get() { return lastData; }

bool start()
{
  PDM.setBufferSize(PdmData::SAMPLE_SIZE);
  PDM.onReceive(on_PDM_data);

  // initialize PDM with:
  // - one channel (mono mode)
  // - a sample rate
  if (!PDM.begin(1, SAMPLE_RATE))
  {
    return false;
  }

  // optionally set the gain, defaults to 20
  PDM.setGain(30);

  fftAnalyzer.reset();
  return true;
}

void stop() { PDM.end(); }

bool processFFT(const PdmData& data, const bool runFFT = true)
{
  if (not data.is_valid())
  {
    return false;
  }

  // get data
  const uint16_t samples = min(PdmData::SAMPLE_SIZE, data.sampleRead);
  for (uint16_t i = 0; i < samples; i++)
  {
    fftAnalyzer.set_data(data.data[i], i);
  }
  // fill the rest with zeros
  for (uint16_t i = data.sampleRead; i < PdmData::SAMPLE_SIZE; i++)
  {
    fftAnalyzer.set_data(0, i);
  }

  if (runFFT)
    fftAnalyzer.FFTcode();

  return true;
}

static SoundStruct soundStruct;

SoundStruct process_fft(const PdmData& data)
{
  // process the sound input
  if (!processFFT(data, true))
  {
    soundStruct.isValid = false;
    return soundStruct;
  }

  // copy the FFT buffer
  soundStruct.isValid = true;
  soundStruct.fftMajorPeakFrequency_Hz = fftAnalyzer.get_major_peak();
  soundStruct.strongestPeakMagnitude = fftAnalyzer.get_magnitude();
  // copy the fft results
  for (uint8_t bandIndex = 0; bandIndex < SoundStruct::numberOfFFtChanels; ++bandIndex)
  {
    soundStruct.fft[bandIndex] = fftAnalyzer.get_fft(bandIndex);
  }
  return soundStruct;
}

} // namespace _private

} // namespace microphone
#endif

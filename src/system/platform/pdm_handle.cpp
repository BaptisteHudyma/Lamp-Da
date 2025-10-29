#ifndef PDM_HANDLE_CPP
#define PDM_HANDLE_CPP

#include "pdm_handle.h"
#include "src/system/platform/time.h"

#include "src/system/utils/fft.h"

#include <PDM.h>

namespace microphone {

PdmData lastData;

// callback every time the microphone reads data
// LEAVE THIS FUNCTION CLEAN, IT'S AN INTERRUPT CALLBACK
void on_PDM_data()
{
  lastData.sampleRead = 0;
  const auto newTime = time_us();

  lastData.sampleDuration_us = newTime - lastData.sampleTime_us;
  lastData.sampleTime_us = newTime;

  // read into the sample buffer. We cast cast our array as an 8bit array of twice the size
  const size_t dataCnt = PDM.available();
  const size_t pdmCnt = min(PdmData::SAMPLE_SIZE * 2, dataCnt);
  const int bytesRead = PDM.read((char*)&lastData.data[0], pdmCnt);

  // number of samples read
  lastData.sampleRead = min(PdmData::SAMPLE_SIZE, bytesRead / 2);
}

namespace _private {

PdmData get()
{
  PdmData copy;
  copy.sampleDuration_us = lastData.sampleDuration_us;
  copy.sampleTime_us = lastData.sampleTime_us;
  copy.sampleRead = lastData.sampleRead;

  float mean = 0.0f;
  for (size_t i = 0; i < copy.sampleRead; i++)
  {
    // add a fake gain (thick plastic body)
    const auto d = lastData.data[i];
    mean += d;
    copy.data[i] = d;
  }
  for (size_t i = copy.sampleRead; i < PdmData::SAMPLE_SIZE; i++)
  {
    copy.data[i] = 0;
  }
  mean /= static_cast<float>(copy.sampleRead);

  // adjust dc voltage offset
  for (size_t i = 0; i < copy.sampleRead; i++)
  {
    copy.data[i] -= mean;
  }

  return copy;
}

bool start()
{
  // PDM handle is created using the pin constants defined in library

  // buffer size shall be 2x the data size
  static_assert(PdmData::SAMPLE_SIZE >= 128 && (PdmData::SAMPLE_SIZE & (PdmData::SAMPLE_SIZE - 1)) == 0,
                "PdmData::SAMPLE_SIZE must be a power of two");
  static_assert(SAMPLE_RATE == 41667 || SAMPLE_RATE == 16000, "PDM application only allow values 41667 or 16000");

  PDM.setBufferSize(PdmData::SAMPLE_SIZE * 2);
  PDM.onReceive(on_PDM_data);

  // initialize PDM with:
  // - one channel (mono mode)
  // - a sample rate (allowed values are 16000 or 41667)
  if (!PDM.begin(1, SAMPLE_RATE))
  {
    return false;
  }

  // optionally set the gain
  // allowed values between 0x00 (-20dB) and 0x50 (+20dB)
  PDM.setGain(0x50);

  return true;
}

void stop() { PDM.end(); }

} // namespace _private

} // namespace microphone
#endif

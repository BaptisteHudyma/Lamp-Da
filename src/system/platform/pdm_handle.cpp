#ifndef PDM_HANDLE_CPP
#define PDM_HANDLE_CPP

#include "pdm_handle.h"
#include "src/system/platform/time.h"

#include "src/system/utils/fft.h"

#include <PDM.h>

namespace microphone {

PdmData lastData;

// callback every time the microphone reads data
void on_PDM_data()
{
  lastData.sampleRead = 0;
  const auto newTime = time_us();

  lastData.sampleDuration_us = newTime - lastData.sampleTime_us;
  lastData.sampleTime_us = newTime;

  // read into the sample buffer. We cast cast our array as an 8bit array of twice the size
  const int bytesRead = PDM.read((char*)&lastData.data[0], PDM.available());

  // number of samples read
  lastData.sampleRead = bytesRead / 2;
}

namespace _private {

PdmData get() { return lastData; }

bool start()
{
  // PDM handle is created using the pin constants defined in library

  // buffer size shall be 2x the data size
  static_assert((PdmData::SAMPLE_SIZE & (PdmData::SAMPLE_SIZE - 1)) == 0,
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
  PDM.setGain(0x40);

  return true;
}

void stop() { PDM.end(); }

} // namespace _private

} // namespace microphone
#endif

#include "sound.h"

#include <cstdint>

#include "src/system/utils/utils.h"
#include "src/system/utils/fft.h"

#include "src/system/platform/time.h"
#include "src/system/platform/gpio.h"
#include "src/system/platform/print.h"

namespace microphone {

FftAnalyzer<PdmData::SAMPLE_SIZE, SoundStruct::numberOfFFtChanels> fftAnalyzer;

bool isStarted = false;
uint32_t lastMicFunctionCall = 0;

bool enable()
{
  lastMicFunctionCall = time_ms();
  if (isStarted)
    return true;

  isStarted = _private::start();
  return isStarted;
}

void disable()
{
  if (!isStarted)
    return;

  _private::stop();
  fftAnalyzer.reset();
  isStarted = false;
}

void disable_after_non_use()
{
  if (isStarted and (time_ms() - lastMicFunctionCall > 1000.0))
  {
    // disable microphone if last reading is old
    disable();
    lampda_print("mic stop: non use");
  }
}

static SoundStruct soundStruct;

SoundStruct process_sound_data(const PdmData& data)
{
  soundStruct.isDataValid = false;
  soundStruct.isFFTValid = false;
  // validity checks
  if (not data.is_valid() or data.sampleRead <= 0 or data.sampleRead > PdmData::SAMPLE_SIZE)
  {
    return soundStruct;
  }

  // process the sound input as FFT
  float sumOfAll = 0.0;
  for (uint16_t i = 0; i < data.sampleRead; i++)
  {
    const int16_t dataP = data.data[i];
    sumOfAll += abs(dataP);

    fftAnalyzer.set_data(dataP, i);

    soundStruct.data[i] = dataP;
  }
  const float dataAverage = sumOfAll / static_cast<float>(data.sampleRead);
  soundStruct.sound_level_Db = 20.0f * log10f(sqrtf(dataAverage));
  soundStruct.isDataValid = true;

  // fill the rest with zeros in FFT
  for (uint16_t i = data.sampleRead; i < PdmData::SAMPLE_SIZE; i++)
  {
    fftAnalyzer.set_data(0, i);
  }

  fftAnalyzer.FFTcode();

  // copy the FFT buffer
  soundStruct.isFFTValid = true;
  soundStruct.fftMajorPeakFrequency_Hz = fftAnalyzer.get_major_peak();
  soundStruct.strongestPeakMagnitude = fftAnalyzer.get_magnitude();
  // copy the fft results
  for (uint8_t bandIndex = 0; bandIndex < SoundStruct::numberOfFFtChanels; ++bandIndex)
  {
    soundStruct.fft[bandIndex] = fftAnalyzer.get_fft(bandIndex);
  }
  // we return a static object here...
  return soundStruct;
}

SoundStruct get_sound_characteristics()
{
  if (!enable())
  {
    // ERROR
    return SoundStruct();
  }

  return process_sound_data(_private::get());
}

} // namespace microphone

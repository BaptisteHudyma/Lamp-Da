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
float autoGain = 1.0f;

static constexpr float minAutoGain = 0.01f;
static constexpr float maxAutoGain = 2.5f;

static constexpr float distortionFactor = 0.0001; //[0, 1]
static constexpr float desiredoutputRMS = 0.001f;

bool enable()
{
  lastMicFunctionCall = time_ms();
  if (isStarted)
    return true;

  autoGain = 1.0f;
  isStarted = _private::start();
  return isStarted;
}

void disable()
{
  if (!isStarted)
    return;

  autoGain = 1.0f;
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

SoundStruct soundStruct;

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
  float dataMedian = 0.0;
  for (uint16_t i = 0; i < data.sampleRead; i++)
  {
    // data is always at zero median, so use absolute
    const int16_t dataP = data.data[i];
    dataMedian += abs(dataP);
    soundStruct.data[i] = dataP;
  }
  const float dataAverage = dataMedian / static_cast<float>(data.sampleRead);

  // if average is greater than sound baseline, update the gain
  const bool shouldUpdateGain = (dataAverage > silenceSoundBaseline);

  // decay gain to 1.0
  if (not shouldUpdateGain)
  {
    static constexpr float restoreGain = 0.1f;
    const float newGain = autoGain - restoreGain * (autoGain - 1.0);
    if (newGain > minAutoGain && newGain < maxAutoGain)
      autoGain = newGain;
  }

  for (uint16_t i = 0; i < data.sampleRead; i++)
  {
    // use auto gain on data
    const float autoGainedData = (soundStruct.data[i] / static_cast<float>(INT16_MAX)) * autoGain;
    if (shouldUpdateGain)
    {
      const float autoGainB = (autoGainedData * autoGainedData) / desiredoutputRMS;
      const float autoGainAA = 1.0f + (distortionFactor * (1.0f - autoGainB));
      autoGain *= autoGainAA;
    }

    soundStruct.rectifiedData[i] = autoGainedData * static_cast<float>(INT16_MAX);
    // FFT on the auto gained data
    fftAnalyzer.set_data(soundStruct.rectifiedData[i], i);
  }
  // keep the gain in bounds
  autoGain = lmpd_constrain(autoGain, minAutoGain, maxAutoGain);

  soundStruct.sound_level_Db = 20.0f * log10f(dataAverage);
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

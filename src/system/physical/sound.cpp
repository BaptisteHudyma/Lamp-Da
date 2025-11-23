#include "sound.h"

#include <cstdint>

#include "src/system/utils/utils.h"
#include "src/system/utils/fft.h"

#include "src/system/platform/time.h"
#include "src/system/platform/gpio.h"
#include "src/system/platform/print.h"

namespace microphone {

FftAnalyzer<PdmData::SAMPLE_SIZE, SoundStruct::numberOfFFtChanels, float> fftAnalyzer;

float SoundStruct::get_fft_resolution_Hz() { return SAMPLE_RATE / static_cast<float>(SAMPLE_SIZE); }

inline float square(const float v) { return v * v; }

float decibel_A_scaling(const float frequency)
{
  return square(12194.0f) * powf(frequency, 4) /
         ((square(frequency) + square(20.6f)) *
          sqrtf((square(frequency) + square(107.7f)) * (square(frequency) + square(737.9f))) *
          (square(frequency) + square(12194.0f)));
}
float decibel_B_scaling(const float frequency)
{
  return square(12194.0f) * powf(frequency, 3) /
         ((square(frequency) + square(20.6f)) * sqrtf((square(frequency) + square(158.5f))) *
          (square(frequency) + square(12194.0f)));
}
float decibel_C_scaling(const float frequency)
{
  return square(12194.0f) * powf(frequency, 2) /
         ((square(frequency) + square(20.6f)) * (square(frequency) + square(12194.0f)));
}

bool isStarted = false;
uint32_t lastMicFunctionCall = 0;
float autoGain = 1.0f;

static constexpr float minAutoGain = 0.0001f;
static constexpr float maxAutoGain = 30.0f;

// reactivity of the filter
static constexpr float distortionFactor = 0.00005f; //[0, 1]
static constexpr float desiredoutputRMS = desiredoutput * desiredoutput;

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

SoundStruct& process_sound_data(const PdmData& data)
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

    // FFT on the raw data
    fftAnalyzer.set_data(dataP, i);
  }
  // fill the rest with zeros in FFT
  for (uint16_t i = data.sampleRead; i < PdmData::SAMPLE_SIZE; i++)
    fftAnalyzer.set_data(0, i);
  const float dataAverage = dataMedian / static_cast<float>(data.sampleRead);

  // if average is greater than sound baseline, update the gain
  const bool shouldUpdateGain = (dataAverage > gainUpdateBaseline);

  // decay gain to 1.0
  if (not shouldUpdateGain)
  {
    static constexpr float restoreGain = 0.1f;
    const float newGain = autoGain - restoreGain * (autoGain - 1.0f);
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
  }
  // keep the gain in bounds
  autoGain = lmpd_constrain(autoGain, minAutoGain, maxAutoGain);

  // run FFT detection
  fftAnalyzer.run_fast_fourrier_transform();

  // copy the FFT buffer
  soundStruct.isFFTValid = true;
  // copy the fft results
  soundStruct.fft_log = fftAnalyzer.fftLog;
  soundStruct.fft_raw = fftAnalyzer.fftBin;

  const float decibelScaling = decibel_A_scaling(fftAnalyzer.maxFrequency);
  // max sound level is actual sound level
  soundStruct.sound_level_Db =
          20.0f * log10f(decibelScaling * fftAnalyzer.maxMagnitude / static_cast<float>(INT16_MAX)) + 110.0f;
  soundStruct.isDataValid = true;

  lampda_print_raw("%f\n", soundStruct.sound_level_Db);

  for (uint16_t i = 0; i < SoundStruct::numberOfFFtChanels; ++i)
  {
    soundStruct.fft_log_end_frequencies[i] = fftAnalyzer.get_log_bin_max_frequency(i);
  }

  // we return a static object here...
  return soundStruct;
}

SoundStruct& get_sound_characteristics()
{
  if (!enable())
  {
    // ERROR
    return soundStruct;
  }

  return process_sound_data(_private::get());
}

} // namespace microphone

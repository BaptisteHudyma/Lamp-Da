#include "sound.h"

#include <cstdint>

#include "src/system/utils/utils.h"

#include "src/system/hal/time.h"
#include "src/system/hal/gpio.h"
#include "src/system/hal/pdm_handle.h"

#include "src/system/bsp/text_out.h"

namespace lampda {
namespace component {
namespace microphone {

common::fft::FftAnalyzer<hal::microphone::PdmData::SAMPLE_SIZE, SoundStruct::numberOfFFtChanels, float> fftAnalyzer;

inline float square(const float v) { return v * v; }

/// Scaling for the DecibelA model
float decibel_A_scaling(const float frequency)
{
  const float frequency2 = square(frequency);
  return square(12194.0f) * square(frequency2) /
         ((square(frequency) + square(20.6f)) *
          sqrtf((square(frequency) + square(107.7f)) * (square(frequency) + square(737.9f))) *
          (square(frequency) + square(12194.0f)));
}
/// Scaling for the DecibelB model
float decibel_B_scaling(const float frequency)
{
  return square(12194.0f) * powf(frequency, 3) /
         ((square(frequency) + square(20.6f)) * sqrtf((square(frequency) + square(158.5f))) *
          (square(frequency) + square(12194.0f)));
}
/// Scaling for the DecibelC model
float decibel_C_scaling(const float frequency)
{
  return square(12194.0f) * powf(frequency, 2) /
         ((square(frequency) + square(20.6f)) * (square(frequency) + square(12194.0f)));
}

/// Set by the inputs (main loop), read by the sound processing (user thread): a single byte write is atomic
volatile uint8_t _sensitivity = defaultSensitivity;

void set_sensitivity(const uint8_t sensitivity)
{
  _sensitivity = lmpd_constrain<uint8_t>(sensitivity, 0, maxSensitivity);
}

uint8_t get_sensitivity() { return _sensitivity; }

float sensitivity_to_gain(const uint8_t sensitivity)
{
  const uint8_t constrained = lmpd_constrain<uint8_t>(sensitivity, 0, maxSensitivity);
  // logarithmic scale, centered on a unity gain
  if (constrained <= defaultSensitivity)
    return powf(minSensitivityGain, (defaultSensitivity - constrained) / static_cast<float>(defaultSensitivity));
  return powf(maxSensitivityGain,
              (constrained - defaultSensitivity) / static_cast<float>(maxSensitivity - defaultSensitivity));
}

bool isStarted = false;
uint32_t lastMicFunctionCall = 0;
float autoGain = 1.0f;

/// Has a sample been processed since the microphone started
bool hasProcessedSample = false;
/// Start time of the last processed sample: the platform can return the same sample many times
uint64_t lastProcessedSampleTime_us = 0;

static constexpr float minAutoGain = 0.0001f;
static constexpr float maxAutoGain = 30.0f;

// reactivity of the filter
static constexpr float distortionFactor = 0.00005f; //[0, 1]
static constexpr float desiredoutputRMS = desiredoutput * desiredoutput;
/// multiplications are faster than divisions on the target
static constexpr float inverseInt16Max = 1.0f / static_cast<float>(INT16_MAX);
static constexpr float inverseDesiredoutputRMS = 1.0f / desiredoutputRMS;

bool enable()
{
  lastMicFunctionCall = hal::time_ms();
  if (isStarted)
    return true;

  autoGain = 1.0f;
  hasProcessedSample = false;
  isStarted = hal::microphone::_private::start();
  return isStarted;
}

void disable()
{
  if (!isStarted)
    return;

  autoGain = 1.0f;
  hasProcessedSample = false;
  hal::microphone::_private::stop();
  fftAnalyzer.reset();
  isStarted = false;
}

void disable_after_non_use()
{
  if (isStarted and (hal::time_ms() - lastMicFunctionCall > 1000.0))
  {
    // disable microphone if last reading is old
    disable();
    bsp::lampda_print("mic stop: non use");
  }
}

SoundStruct soundStruct;

/// Fill the constant parts of the sound structure
static bool init_sound_struct()
{
  for (uint16_t i = 0; i < SoundStruct::numberOfFFtChanels; ++i)
  {
    soundStruct.fft_log_end_frequencies[i] = fftAnalyzer.get_log_bin_max_frequency(i);
  }
  return true;
}

SoundStruct& process_sound_data(const hal::microphone::PdmData& data)
{
  static const bool isSoundStructInit = init_sound_struct();
  (void)isSoundStructInit;

  // validity checks
  if (not data.is_valid() or data.sampleRead > hal::microphone::PdmData::SAMPLE_SIZE)
  {
    soundStruct.isDataValid = false;
    soundStruct.isFFTValid = false;
    return soundStruct;
  }

  // this sample was already processed: the results are the same, skip the FFT
  if (hasProcessedSample and data.sampleTime_us == lastProcessedSampleTime_us)
  {
    return soundStruct;
  }
  hasProcessedSample = true;
  lastProcessedSampleTime_us = data.sampleTime_us;

  // user sensitivity, applied on the raw samples
  const float sensitivityGain = sensitivity_to_gain(_sensitivity);
  const bool hasSensitivityGain = sensitivityGain != 1.0f;

  // process the sound input as FFT
  int32_t dataSum = 0;
  for (uint16_t i = 0; i < data.sampleRead; i++)
  {
    int16_t dataP = data.data[i];
    if (hasSensitivityGain)
    {
      dataP = static_cast<int16_t>(
              lmpd_constrain(dataP * sensitivityGain, static_cast<float>(-INT16_MAX), static_cast<float>(INT16_MAX)));
    }
    // data is always at zero median, so use absolute
    dataSum += (dataP < 0) ? -dataP : dataP;
    soundStruct.data[i] = dataP;

    // FFT on the raw data
    fftAnalyzer.set_data(dataP, i);
  }
  // fill the rest with zeros in FFT
  for (uint16_t i = data.sampleRead; i < hal::microphone::PdmData::SAMPLE_SIZE; i++)
    fftAnalyzer.set_data(0, i);
  const float dataAverage = static_cast<float>(dataSum) / static_cast<float>(data.sampleRead);

  // if average is greater than sound baseline, update the gain
  const bool shouldUpdateGain = (dataAverage > hal::microphone::gainUpdateBaseline);

  if (shouldUpdateGain)
  {
    for (uint16_t i = 0; i < data.sampleRead; i++)
    {
      // use auto gain on data
      const float autoGainedData = (soundStruct.data[i] * inverseInt16Max) * autoGain;
      const float autoGainB = (autoGainedData * autoGainedData) * inverseDesiredoutputRMS;
      autoGain *= 1.0f + (distortionFactor * (1.0f - autoGainB));

      soundStruct.rectifiedData[i] = autoGainedData * static_cast<float>(INT16_MAX);
    }
    // keep the gain in bounds
    autoGain = lmpd_constrain(autoGain, minAutoGain, maxAutoGain);
  }
  else
  {
    // decay gain to 1.0
    static constexpr float restoreGain = 0.1f;
    const float newGain = autoGain - restoreGain * (autoGain - 1.0f);
    if (newGain > minAutoGain && newGain < maxAutoGain)
      autoGain = newGain;

    // constant gain over the whole sample
    for (uint16_t i = 0; i < data.sampleRead; i++)
    {
      soundStruct.rectifiedData[i] = soundStruct.data[i] * autoGain;
    }
  }

  // run FFT detection
  fftAnalyzer.run_fast_fourrier_transform();

  // copy the fft results
  soundStruct.fft_log = fftAnalyzer.fftLog;
  soundStruct.fft_raw = fftAnalyzer.fftBin;
  soundStruct.isFFTValid = true;

  const float decibelScaling = decibel_A_scaling(fftAnalyzer.maxFrequency);
  // max sound level is actual sound level
  soundStruct.sound_level_Db = 20.0f * log10f(decibelScaling * fftAnalyzer.maxMagnitude * inverseInt16Max) + 110.0f;
  soundStruct.maxAmplitude = fftAnalyzer.maxMagnitude;
  soundStruct.maxAmplitudeFrequency = fftAnalyzer.maxFrequency;
  soundStruct.isDataValid = true;

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

  return process_sound_data(hal::microphone::_private::get());
}

} // namespace microphone
} // namespace component
} // namespace lampda

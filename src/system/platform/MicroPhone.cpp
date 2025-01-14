#ifndef MICROPHONE_IMPL_CPP
#define MICROPHONE_IMPL_CPP

#include "MicroPhone.h"

#include <PDM.h>

#include <cstdint>

#include "fft.h"
#include "src/system/ext/noise.h"
#include "src/system/ext/random8.h"
#include "src/system/utils/constants.h"

#include "src/system/platform/gpio.h"
#include "src/system/platform/time.h"

namespace microphone {

// buffer to read samples into, each sample is 16-bits
constexpr size_t SAMPLE_SIZE = 512; // samplesFFT;
int16_t _sampleBuffer[SAMPLE_SIZE];
volatile uint16_t samplesRead;

FftAnalyzer<SAMPLE_SIZE, numberOfFFtChanels> fftAnalyzer;

uint32_t lastMeasurmentMicros;
uint32_t lastMeasurmentDurationMicros;
void on_PDM_data()
{
  const uint32_t newTime = time_us();
  lastMeasurmentDurationMicros = newTime - lastMeasurmentMicros;
  lastMeasurmentMicros = newTime;
  // query the number of bytes available
  const uint16_t bytesAvailable = PDM.available();

  // read into the sample buffer
  PDM.read((char*)&_sampleBuffer[0], bytesAvailable);

  // number of samples read
  samplesRead = bytesAvailable / 2;
}

static uint32_t lastMicFunctionCall = 0;
static bool isStarted = false;

void enable()
{
  lastMicFunctionCall = time_ms();
  if (isStarted)
  {
    return;
  }

  DigitalPin(DigitalPin::GPIO::microphonePower).set_high(true);

  PDM.setBufferSize(512);
  PDM.onReceive(on_PDM_data);

  // initialize PDM with:
  // - one channel (mono mode)
  // - a sample rate
  if (!PDM.begin(1, SAMPLE_RATE))
  {
    // block program execution
    while (1)
    {
      delay_ms(1000);
    }
  }

  // optionally set the gain, defaults to 20
  PDM.setGain(30);

  fftAnalyzer.reset();

  isStarted = true;
}

void disable()
{
  if (!isStarted)
  {
    return;
  }

  PDM.end();

  DigitalPin(DigitalPin::GPIO::microphonePower).set_high(false);
  isStarted = false;
}

void disable_after_non_use()
{
  if (isStarted and (time_ms() - lastMicFunctionCall > 1000.0))
  {
    // disable microphone if last reading is old
    disable();
  }
}

float get_sound_level_Db()
{
  enable();
  if (!isStarted)
  {
    // ERROR
    return 0.0;
  }

  static float lastValue = 0;

  if (!samplesRead)
    return lastValue;

  float sumOfAll = 0.0;
  const uint16_t samples = min(SAMPLE_SIZE, samplesRead);
  for (uint16_t i = 0; i < samples; i++)
  {
    sumOfAll += powf(_sampleBuffer[i] / (float)1024.0, 2.0);
  }
  const float average = sumOfAll / (float)samples;

  lastValue = 20.0 * log10f(sqrtf(average));
  // convert to decibels
  return lastValue;
}

bool processFFT(const bool runFFT = true)
{
  enable();
  if (!isStarted)
  {
    // ERROR
    return false;
  }

  if (!samplesRead)
  {
    return false;
  }

  // get data
  const uint16_t samples = min(SAMPLE_SIZE, samplesRead);
  for (uint16_t i = 0; i < samples; i++)
  {
    fftAnalyzer.set_data(_sampleBuffer[i], i);
  }
  // fill the rest with zeros
  for (uint16_t i = samplesRead; i < SAMPLE_SIZE; i++)
  {
    fftAnalyzer.set_data(0, i);
  }

  samplesRead = 0;
  if (runFFT)
    fftAnalyzer.FFTcode();

  return true;
}

static SoundStruct soundStruct;

SoundStruct get_fft()
{
  // process the sound input
  if (!processFFT(true))
  {
    soundStruct.isValid = false;
    return soundStruct;
  }

  // copy the FFT buffer
  soundStruct.isValid = true;
  soundStruct.fftMajorPeakFrequency_Hz = fftAnalyzer.get_major_peak();
  soundStruct.strongestPeakMagnitude = fftAnalyzer.get_magnitude();
  // copy the fft results
  for (uint8_t bandIndex = 0; bandIndex < numberOfFFtChanels; ++bandIndex)
  {
    soundStruct.fft[bandIndex] = fftAnalyzer.get_fft(bandIndex);
  }
  return soundStruct;
}

} // namespace microphone

#endif
/**
 * Test for the fast fourrier transform
 */
#include <cmath>
#include <gtest/gtest.h>
#include "src/system/utils/fft.h"

using namespace std::chrono_literals;
static constexpr double samplingFrequency = static_cast<double>(SAMPLE_RATE);

double generate_sin_frequency(const double frequency,
                              const double amplitude,
                              const double index,
                              const double offset = 0.0)
{
  return amplitude * sin(index * 2.0f * M_PI * frequency / samplingFrequency + offset);
}

TEST(fastFourrierTest, simpleSignalAmplitude)
{
  const size_t samples = 512;
  const size_t numberOfBins = 32;

  const float freqBinSize = samplingFrequency / static_cast<float>(samples);
  const float frequency = 440.0;

  const float amplitude = 255.0;

  FftAnalyzer<samples, numberOfBins> fftAnalyzer;
  for (uint16_t i = 0; i < samples; i++)
    fftAnalyzer.set_data(generate_sin_frequency(frequency, amplitude, i), i);
  fftAnalyzer.run_fast_fourrier_transform();

  // check that we found the correct amplitude
  const int frequIndex = fftAnalyzer.to_bin_index(frequency);
  EXPECT_NEAR(fftAnalyzer.fftBin[frequIndex], amplitude, amplitude * 0.05);
  // check that the bins around this one have lesser amplitudes
  EXPECT_LT(fftAnalyzer.fftBin[frequIndex - 1], fftAnalyzer.fftBin[frequIndex]);
  EXPECT_LT(fftAnalyzer.fftBin[frequIndex + 1], fftAnalyzer.fftBin[frequIndex]);

  // check the logarithm bin values
  for (size_t i = 0; i < numberOfBins; i++)
  {
    const float minLogFrequency = fftAnalyzer.get_log_bin_min_frequency(i);
    const float maxLogFrequency = fftAnalyzer.get_log_bin_max_frequency(i);
    if (frequency >= minLogFrequency && frequency <= maxLogFrequency)
      EXPECT_NEAR(fftAnalyzer.fftLog[i], amplitude, amplitude * 0.05);
    else
      EXPECT_LT(fftAnalyzer.fftLog[i], amplitude * 0.2);
  }
}

TEST(fastFourrierTest, composedSignalAmplitude)
{
  const size_t samples = 1024;
  const size_t numberOfBins = 32;

  const float freqBinSize = samplingFrequency / static_cast<float>(samples);

  const float frequency1 = 20.0;
  const float amplitude1 = 12.0;

  const float frequency2 = 220.0;
  const float amplitude2 = 5.0;

  const float frequency3 = 300.0;
  const float amplitude3 = 4.0;

  FftAnalyzer<samples, numberOfBins> fftAnalyzer;
  for (uint16_t i = 0; i < samples; i++)
    fftAnalyzer.set_data(5 + generate_sin_frequency(frequency1, amplitude1, i, -0.3) +
                                 generate_sin_frequency(frequency2, amplitude2, i, -0.5) +
                                 generate_sin_frequency(frequency3, amplitude3, i, -1.5),
                         i);
  fftAnalyzer.run_fast_fourrier_transform();

  // check that we found the correct amplitude
  const int frequ1Index = fftAnalyzer.to_bin_index(frequency1);
  EXPECT_NEAR(fftAnalyzer.fftBin[frequ1Index], amplitude1, amplitude1 * 0.2);
  // check that the bins around this one have lesser amplitudes
  EXPECT_LT(fftAnalyzer.fftBin[frequ1Index - 1], fftAnalyzer.fftBin[frequ1Index]);
  EXPECT_LT(fftAnalyzer.fftBin[frequ1Index + 1], fftAnalyzer.fftBin[frequ1Index]);

  // check that we found the correct amplitude
  const int frequ2Index = fftAnalyzer.to_bin_index(frequency2);
  EXPECT_NEAR(fftAnalyzer.fftBin[frequ2Index], amplitude2, amplitude2 * 0.2);
  // check that the bins around this one have lesser amplitudes
  EXPECT_LT(fftAnalyzer.fftBin[frequ2Index - 1], fftAnalyzer.fftBin[frequ2Index]);
  EXPECT_LT(fftAnalyzer.fftBin[frequ2Index + 1], fftAnalyzer.fftBin[frequ2Index]);

  // check that we found the correct amplitude
  const int frequ3Index = fftAnalyzer.to_bin_index(frequency3);
  EXPECT_NEAR(fftAnalyzer.fftBin[frequ3Index], amplitude3, amplitude3 * 0.2);
  // check that the bins around this one have lesser amplitudes
  EXPECT_LT(fftAnalyzer.fftBin[frequ3Index - 1], fftAnalyzer.fftBin[frequ3Index]);
  EXPECT_LT(fftAnalyzer.fftBin[frequ3Index + 1], fftAnalyzer.fftBin[frequ3Index]);

  for (size_t i = 0; i < numberOfBins; i++)
  {
    const float minLogFrequency = fftAnalyzer.get_log_bin_min_frequency(i);
    const float maxLogFrequency = fftAnalyzer.get_log_bin_max_frequency(i);
    if (frequency1 >= minLogFrequency && frequency1 <= maxLogFrequency)
      EXPECT_NEAR(fftAnalyzer.fftLog[i], amplitude1, amplitude1 * 2);
    else if (frequency2 >= minLogFrequency && frequency2 <= maxLogFrequency)
      EXPECT_GT(fftAnalyzer.fftLog[i], amplitude2 * 0.8);
    else if (frequency3 >= minLogFrequency && frequency3 <= maxLogFrequency)
      EXPECT_GT(fftAnalyzer.fftLog[i], amplitude3 * 0.8);
    else
      EXPECT_LT(fftAnalyzer.fftLog[i], amplitude2);
  }
}

TEST(fastFourrierTest, simpleSignalAmplitudeSweep)
{
  const size_t samples = 512;
  const size_t numberOfBins = 32;

  const float freqBinSize = samplingFrequency / static_cast<float>(samples);

  const float amplitude = 1000.0;

  FftAnalyzer<samples, numberOfBins> fftAnalyzer;
  // the first 2 bins have unreliable signals
  for (size_t freqSample = 2; freqSample < (samples / 2) - 1; freqSample++)
  {
    const float frequency = freqBinSize * freqSample;

    fftAnalyzer.reset();
    for (uint16_t i = 0; i < samples; i++)
      fftAnalyzer.set_data(generate_sin_frequency(frequency, amplitude, i), i);
    fftAnalyzer.run_fast_fourrier_transform();

    // check that we found the correct amplitude
    EXPECT_EQ(fftAnalyzer.to_bin_index(frequency), freqSample);
    EXPECT_NEAR(fftAnalyzer.fftBin[freqSample], amplitude, amplitude * 0.05);
    // check that the bins around this one have lesser amplitudes
    EXPECT_LT(fftAnalyzer.fftBin[freqSample - 1], fftAnalyzer.fftBin[freqSample]);
    EXPECT_LT(fftAnalyzer.fftBin[freqSample + 1], fftAnalyzer.fftBin[freqSample]);

    // check the logarithm bin values
    for (size_t i = 0; i < numberOfBins; i++)
    {
      const float minLogFrequency = fftAnalyzer.get_log_bin_min_frequency(i);
      const float maxLogFrequency = fftAnalyzer.get_log_bin_max_frequency(i);
      if (frequency >= minLogFrequency && frequency <= maxLogFrequency)
        EXPECT_NEAR(fftAnalyzer.fftLog[i], amplitude, amplitude * 0.05);
      else
        EXPECT_NEAR(fftAnalyzer.fftLog[i], 0.0, amplitude * 0.05);
    }
  }
}

TEST(fastFourrierTest, composedSignalAmplitudeSweep)
{
  const size_t samples = 512;
  const size_t numberOfBins = 16;

  const float freqBinSize = samplingFrequency / static_cast<float>(samples);

  const float amplitude1 = 100.0;
  const float amplitude2 = 50.0;
  const float amplitude3 = 30.0;

  FftAnalyzer<samples, numberOfBins> fftAnalyzer;
  // the first 2 bins have unreliable signals
  for (size_t freqSample = 2; freqSample < (samples / 2) - 1; freqSample++)
  {
    const float frequency1 = freqBinSize * freqSample;
    const float frequency2 = freqBinSize * freqSample * 2.5;
    const float frequency3 = freqBinSize * freqSample * 0.2;

    fftAnalyzer.reset();
    for (uint16_t i = 0; i < samples; i++)
      fftAnalyzer.set_data(generate_sin_frequency(frequency1, amplitude1, i, -0.3) +
                                   generate_sin_frequency(frequency2, amplitude2, i, -0.8) +
                                   generate_sin_frequency(frequency3, amplitude3, i, -1.5),
                           i);

    fftAnalyzer.run_fast_fourrier_transform();

    // check that we found the correct amplitude
    EXPECT_EQ(fftAnalyzer.to_bin_index(frequency1), freqSample);
    EXPECT_NEAR(fftAnalyzer.fftBin[freqSample], amplitude1, amplitude1 * 0.2);
    // check that the bins around this one have lesser amplitudes
    EXPECT_LT(fftAnalyzer.fftBin[freqSample - 1], fftAnalyzer.fftBin[freqSample]);
    EXPECT_LT(fftAnalyzer.fftBin[freqSample + 1], fftAnalyzer.fftBin[freqSample]);

    // check the logarithm bin values
    for (size_t i = 0; i < numberOfBins; i++)
    {
      const float minLogFrequency = fftAnalyzer.get_log_bin_min_frequency(i);
      const float maxLogFrequency = fftAnalyzer.get_log_bin_max_frequency(i);
      if (frequency1 >= minLogFrequency && frequency1 <= maxLogFrequency)
        EXPECT_GT(fftAnalyzer.fftLog[i], amplitude1 * 0.8);
    }
  }
}

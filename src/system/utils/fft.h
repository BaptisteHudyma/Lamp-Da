#pragma once

// WARNING Sound reactive variables that are used by the animations or other
// asynchronous routines must NOT have interim values, but only updated in a
// single calculation. These are:
//
// fftBin[]   fftResult[]   FFT_MajorPeak   FFT_Magnitude
//
// Otherwise, the animations may asynchronously read interim values of these
// variables.
//

#include <array>
#include <cmath>
#include <cstdint>

#define FFT_SQRT_APPROXIMATION
#define FFT_SPEED_OVER_PRECISION
#include "src/depends/arduinoFFT/src/arduinoFFT.h"

#include "src/system/utils/utils.h"

constexpr int SAMPLE_RATE = 16000; // Base sample rate in Hz - standard.

/**
 * samplesFFT Is the input frequency bins before analysis
 * resultSize Is the resulting frequency bins after analysis
 */
template<uint16_t samplesFFT, uint16_t resultSize> class FftAnalyzer
{
  /// output variables
public:
  float FFT_MajorPeak = 1.0f;               // major peak in hertz
  float FFT_Magnitude = 0.0001f;            // major peak magnitude
  std::array<float, samplesFFT / 2> fftBin; // raw fft results
  std::array<float, resultSize> fftLog;     // fft result as a log scale

  ///

private:
  // use this to convert bins to frequencies
  std::array<uint16_t, resultSize + 1> minFrequenciesPerBin_log;

  // map fft index to log index
  std::array<float, resultSize> _numSamplesPerBar_logPoolSize;
  std::array<uint16_t, samplesFFT / 2> _numSamplesPerBar_log;

  // FFT entry point
  float vReal[samplesFFT];
  float vImag[samplesFFT];
  ArduinoFFT<float> FFT = ArduinoFFT<float>(vReal, vImag, samplesFFT, SAMPLE_RATE, true);

public:
  void reset()
  {
    FFT_Magnitude = 0;
    FFT_MajorPeak = 1;
    fftLog.fill(0);
    fftBin.fill(0);
  }

  FftAnalyzer()
  {
    // fill the initial variables
    // ]1; +oo[, closer to 1 get closer to a linear scale
    static constexpr float logMultiplier = 1.2f;
    static constexpr float multiplier = samplesFFT / static_cast<float>(SAMPLE_RATE);

    _numSamplesPerBar_log.fill(0);
    uint16_t indexLow = 0;
    // calculate octave frequencies
    for (uint16_t octave = 1; octave < resultSize + 1; ++octave)
    {
      const float freqOctave = (SAMPLE_RATE / 2.0) / powf(logMultiplier, resultSize - octave);

      const uint16_t indexHigh = freqOctave * multiplier;
      minFrequenciesPerBin_log[octave] = floor(freqOctave);

      const uint16_t range = indexHigh - indexLow;
      for (int16_t i = indexLow; i < indexHigh; ++i)
      {
        _numSamplesPerBar_log[i] = octave - 1;
      }
      _numSamplesPerBar_logPoolSize[octave - 1] = 1.0f / static_cast<float>(range);

      indexLow = indexHigh;
    }

    // reset variables
    reset();
  }

  /**
   * \brief get the minimum frequency represented by the fft bin at index
   * \param[index] index of the fft bin
   * \return the min frequency of the bin
   */
  uint16_t get_log_bin_min_frequency(uint16_t index) const noexcept
  {
    if (index >= resultSize)
      index = resultSize - 1;
    return minFrequenciesPerBin_log[index];
  }
  /**
   * \brief get the minimum frequency represented by the fft bin at index
   * \param[index] index of the fft bin
   * \return the max frequency of the bin
   */
  uint16_t get_log_bin_max_frequency(uint16_t index) const noexcept
  {
    // get next bin min, it will be this bin max
    index += 1;
    if (index >= resultSize + 1)
      index = resultSize;
    return minFrequenciesPerBin_log[index];
  }

  inline void set_data(const int16_t data, uint16_t index)
  {
    if (index >= samplesFFT)
      index = samplesFFT - 1;

    vReal[index] = data;
    vImag[index] = 0;
  }

  // FFT main code
  void FFTcode()
  {
    // dc removal: already done
    // FFT.dcRemoval();
    // windowing
    FFT.windowing(FFTWindow::Blackman_Harris, FFTDirection::Forward);
    // compute results
    FFT.compute(FFTDirection::Forward);
    FFT.complexToMagnitude();

    // store max frequency
    FFT.majorPeak(&FFT_MajorPeak, &FFT_Magnitude);
    FFT_Magnitude = fabsf(FFT_Magnitude);

    // store result
    // fill octaves
    fftLog.fill(0);
    for (uint16_t i = 0; i < samplesFFT / 2; ++i)
    {
      const float t = fabsf(vReal[i]);
      fftBin[i] = t;

      const auto octave = _numSamplesPerBar_log[i];
      fftLog[octave] += t;
    }
    for (uint16_t octave = 0; octave < resultSize; ++octave)
    {
      fftLog[octave] *= _numSamplesPerBar_logPoolSize[octave];
    }
  } // FFTcode()
};

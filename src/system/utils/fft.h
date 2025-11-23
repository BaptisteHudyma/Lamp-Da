#pragma once

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
template<uint16_t samplesFFT, uint16_t resultSize, typename T = float> class FftAnalyzer
{
  static_assert(samplesFFT > 0 && (samplesFFT & (samplesFFT - 1)) == 0, "samplesFFT must be a power of two");
  static constexpr uint16_t samplesFFTRes = samplesFFT >> 1;
  /// output variables
public:
  std::array<T, samplesFFTRes> fftBin; // raw fft results
  std::array<T, resultSize> fftLog;    // fft result as a log scale
  T maxMagnitude;
  T maxFrequency;

  ///

private:
  // use this to convert bins to frequencies
  std::array<T, resultSize + 1> minFrequenciesPerBin_log;

  // map fft index to log index
  std::array<uint16_t, samplesFFTRes> _numSamplesPerBar_log;
  const FFTWindow fftWindowing = FFTWindow::Rectangle;
  T windowSum = 0.0; /// computed window sum

  // FFT entry point
  T vReal[samplesFFT];
  T vImag[samplesFFT];
  ArduinoFFT<T> FFT = ArduinoFFT<T>(vReal, vImag, samplesFFT, SAMPLE_RATE, true);

public:
  void reset()
  {
    fftLog.fill(0);
    fftBin.fill(0);
  }

  FftAnalyzer()
  {
    // fill the initial variables
    // ]1; +oo[, closer to 1 get closer to a linear scale
    static constexpr T logMultiplier = 1.2;

    _numSamplesPerBar_log.fill(0);
    minFrequenciesPerBin_log[0] = 0.0;
    // calculate octave frequencies
    for (uint16_t octave = 1; octave < resultSize; ++octave)
    {
      assert(octave < minFrequenciesPerBin_log.size());
      // center frequency of this bin
      minFrequenciesPerBin_log[octave] = (SAMPLE_RATE / 2.0) / pow(logMultiplier, resultSize - octave);
    }
    // set last max bin
    minFrequenciesPerBin_log[resultSize] = (SAMPLE_RATE / 2.0);

    // fill the linear to bin to log bin
    _numSamplesPerBar_log[0] = 0;
    for (uint16_t i = 1; i < samplesFFTRes; ++i)
    {
      const T freq = medium_bin_frequency(i);
      uint16_t octave = 0;
      for (; octave < resultSize; ++octave)
      {
        if (freq > get_log_bin_min_frequency(octave) && freq <= get_log_bin_max_frequency(octave))
          break;
      }
      assert(octave >= 0 && octave < resultSize);
      _numSamplesPerBar_log[i] = octave;
    }

    // compute window sum
    // work on half the data for optimization
    for (uint16_t i = 0; i < samplesFFTRes; ++i)
      set_data(1.0, i);
    FFT.windowing(fftWindowing, FFTDirection::Forward);
    windowSum = 0.0;
    for (uint16_t i = 0; i < samplesFFTRes; ++i)
      windowSum += vReal[i] * 2;

    // reset variables
    reset();
  }

  // frequency to bin index
  int to_bin_index(const T frequency) const noexcept
  {
    return round(frequency / (static_cast<T>(SAMPLE_RATE) / samplesFFT));
  }
  /// center frequency of a linear bin
  T medium_bin_frequency(uint16_t index) const noexcept { return index * SAMPLE_RATE / static_cast<T>(samplesFFT); }
  /// min frequency of a linear bin
  T min_bin_frequency(uint16_t index) const noexcept
  {
    if (index == 0)
      return 0.0;
    return max_bin_frequency(index - 1);
  }
  /// max frequency of a linear bin
  T max_bin_frequency(uint16_t index) const noexcept
  {
    return (index + 0.5) * SAMPLE_RATE / static_cast<T>(samplesFFT);
  }

  /**
   * \brief get the minimum frequency represented by the fft bin at index
   * \param[in] index index of the fft bin
   * \return the min frequency of the bin
   */
  T get_log_bin_min_frequency(uint16_t index) const noexcept
  {
    if (index >= minFrequenciesPerBin_log.size())
      index = minFrequenciesPerBin_log.size() - 1;
    return minFrequenciesPerBin_log[index];
  }
  /**
   * \brief get the minimum frequency represented by the fft bin at index
   * \param[in] index index of the fft bin
   * \return the max frequency of the bin
   */
  T get_log_bin_max_frequency(uint16_t index) const noexcept
  {
    // get next bin min, it will be this bin max
    return get_log_bin_min_frequency(index + 1);
  }

  inline void set_data(const T data, uint16_t index)
  {
    if (index >= samplesFFT)
      index = samplesFFT - 1;

    vReal[index] = data;
    vImag[index] = 0;
  }

  // FFT main code
  void run_fast_fourrier_transform()
  {
    // recenter all data around zero
    FFT.dcRemoval();

    FFT.windowing(fftWindowing, FFTDirection::Forward);
    FFT.compute(FFTDirection::Forward);
    FFT.complexToMagnitude();

    // store result
    fftLog.fill(0);
    maxMagnitude = 0.0;
    maxFrequency = 0.0;
    for (uint16_t i = 0; i < samplesFFTRes; ++i)
    {
      const T t = fabsf(vReal[i]) * 2.0 / windowSum;
      fftBin[i] = t;

      const uint16_t octave = _numSamplesPerBar_log[i];
      assert(octave >= 0 && octave < fftLog.size());

      // we prefer to keep the max every time, to not squash important frequencies in large bins
      fftLog[octave] = max<T>(fftLog[octave], t);
      if (t > maxMagnitude)
      {
        maxMagnitude = t;
        maxFrequency = medium_bin_frequency(i);
      }
    }
  } // run_fast_fourrier_transform()
};

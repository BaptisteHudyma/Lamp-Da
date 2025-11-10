#pragma once

/**
 * Based on the sound reactive fork to WLED.
 * Modified to be more flexible with different frequency bin sizes
 */

/*
 * This file allows you to add own functionality to WLED more easily
 * See: https://github.com/Aircoookie/WLED/wiki/Add-own-functionality
 * EEPROM bytes 2750+ are reserved for your custom use case. (if you extend
 * #define EEPSIZE in const.h) bytes 2400+ are currently ununsed, but might be
 * used for future wled features
 */

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

#include "arduinoFFT.h"

#include "src/system/utils/utils.h"

constexpr int SAMPLE_RATE = 16000; // Base sample rate in Hz - standard.
                                   // Physical sample time -> 50ms
// constexpr int SAMPLE_RATE = 20480;            // Base sample rate in Hz -
// 20Khz is experimental.    Physical sample time -> 25ms constexpr int
// SAMPLE_RATE = 22050;            // Base sample rate in Hz - 22Khz is a
// standard rate. Physical sample time -> 23ms

/**
 * samplesFFT Is the input frequency bins before analysis
 * fftResCount Is the resulting frequency bins after analysis
 */
template<uint16_t samplesFFT, uint16_t fftResCount> class FftAnalyzer
{
private:
  static constexpr unsigned useInputFilter = 0; // if >0 , enables a bandpass filter 80Hz-8Khz
                                                // to remove noise. Applies before FFT.

  static constexpr uint8_t inputLevel = 128; // UI slider value

#ifndef SR_SQUELCH
  static constexpr uint8_t soundSquelch = 10.0; // squelch value for volume reactive routines (config value)
#else
  static constexpr uint8_t soundSquelch = SR_SQUELCH; // squelch value for volume reactive routines (config value)
#endif

#ifndef SR_GAIN
  static constexpr uint8_t sampleGain = 60; // sample gain (config value)
#else
  static constexpr uint8_t sampleGain = SR_GAIN; // sample gain (config value)
#endif

  static constexpr bool enableAutoGain = true; // Automagic gain control

  static constexpr uint16_t minFrequency = 60;
  static constexpr uint16_t maxFrequency = 5120;

  // FFT Variables
  float FFT_MajorPeak = 1.0f;
  float FFT_Magnitude = 0.0001;

  // These are the input and output vectors.  Input vectors receive computed
  // results from FFT.
  float windowWeighingFactors[samplesFFT];
  std::array<float, samplesFFT> fftBin;

  // valid indexes in the fft bins
  static const uint16_t startValidBin = 3;

  // the bin/frequency multiplier
  static constexpr float multiplier = pow(maxFrequency / (float)minFrequency, 1 / (float)fftResCount);

  // Try and normalize fftBin values to a max of 4096, so that 4096/16 = 256.
  // Oh, and bins 0,1,2 are no good, so we'll zero them out.
  std::array<float, fftResCount> fftCalc;
  std::array<uint8_t, fftResCount> fftResult;  // Our calculated result table
  std::array<float, fftResCount> fftResultMax; // A table used for testing to determine how
                                               // our post-processing is working.
  std::array<float, fftResCount> fftAvg;

  // use this to convert bins to frequencies
  std::array<uint16_t, fftResCount> minFrequenciesPerBin;
  std::array<uint16_t, fftResCount> maxFrequenciesPerBin;

  void fill_frequency_bins()
  {
    uint16_t frequency = minFrequency;
    for (uint16_t i = 0; i < fftResCount; ++i)
    {
      const uint16_t newFrequ = round(frequency * multiplier);
      minFrequenciesPerBin[i] = frequency;
      maxFrequenciesPerBin[i] = newFrequ;
      frequency = newFrequ;
    }
  }

  /**
   * Approximation of the linear noise correction factor
   */
  float compute_linear_noise(uint16_t binIndex)
  {
    // magix numbers found by trial and error
    static constexpr float linearNoiseMultiplier = 0.03;
    static constexpr float linearNoiseExponentMultiplier = 3.1;
    return 1.0 / (linearNoiseMultiplier * expf(linearNoiseExponentMultiplier * binIndex / (float)fftResCount));
  }

  ////////////////////
  // Begin FFT Code //
  ////////////////////

  constexpr float fftMedian(uint from, uint to)
  {
    uint i = from;
    float result = 0;
    while (i <= to and i < samplesFFT)
    {
      result += fftBin[++i];
    }
    return result / ((to - from) + 1);
  }

  // Bandpass filter for PDM microphones
  void runMicFilter(uint16_t numSamples, float* sampleBuffer)
  { // pre-filtering of raw samples (band-pass)
    // band pass filter - can reduce noise floor by a factor of 50
    // downside: frequencies below 60Hz will be ignored

    // low frequency cutoff parameter - see
    // https://dsp.stackexchange.com/questions/40462/exponential-moving-average-cut-off-frequency
    // constexpr float alpha = 0.062f;   // 100Hz
    constexpr float alpha = 0.04883f; //  80Hz
    // constexpr float alpha = 0.03662f; //  60Hz
    // constexpr float alpha = 0.0225f;  //  40Hz
    //  high frequency cutoff  parameter
    // constexpr float beta1 = 0.75;    //  5Khz
    // constexpr float beta1 = 0.82;    //  7Khz
    constexpr float beta1 = 0.8285; //  8Khz
    // constexpr float beta1 = 0.85;    // 10Khz

    constexpr float beta2 = (1.0f - beta1) / 2.0;
    static float last_vals[2] = {0.0f}; // FIR high freq cutoff filter
    static float lowfilt = 0.0f;        // IIR low frequency cutoff filter

    for (int i = 0; i < numSamples; i++)
    {
      // FIR lowpass, to remove high frequency noise
      float highFilteredSample;
      if (i < (numSamples - 1))
        highFilteredSample =
                beta1 * sampleBuffer[i] + beta2 * last_vals[0] + beta2 * sampleBuffer[i + 1]; // smooth out spikes
      else
        highFilteredSample = beta1 * sampleBuffer[i] + beta2 * last_vals[0] +
                             beta2 * last_vals[1]; // spcial handling for last sample in array
      last_vals[1] = last_vals[0];
      last_vals[0] = sampleBuffer[i];
      sampleBuffer[i] = highFilteredSample;
      // IIR highpass, to remove low frequency noise
      lowfilt += alpha * (sampleBuffer[i] - lowfilt);
      sampleBuffer[i] = sampleBuffer[i] - lowfilt;
    }
  }

  // sample smoothing, by using a sliding average FIR highpass filter (first
  // half of MicFilter from above)
  void runMicSmoothing(uint16_t numSamples, float* sampleBuffer)
  {
    constexpr float beta1 = 0.8285;               //  ~8Khz
    constexpr float beta2 = (1.0f - beta1) / 2.0; // note to self: better use biquad ?
    static float last_vals[2] = {0.0f};           // FIR filter buffer

    for (int i = 0; i < numSamples; i++)
    {
      float highFilteredSample;
      if (i < (numSamples - 1))
        highFilteredSample =
                beta1 * sampleBuffer[i] + beta2 * last_vals[0] + beta2 * sampleBuffer[i + 1]; // smooth out spikes
      else
        highFilteredSample = beta1 * sampleBuffer[i] + beta2 * last_vals[0] +
                             beta2 * last_vals[1]; // spcial handling for last sample in array
      last_vals[1] = last_vals[0];
      last_vals[0] = sampleBuffer[i];
      sampleBuffer[i] = highFilteredSample;
    }
  }

  // a variation of above, with higher cut-off frequency
  void runMicSmoothing_v2(uint16_t numSamples, float* sampleBuffer)
  {
    constexpr float beta1 = 0.85;                 // ~10Khz
    constexpr float beta2 = (1.0f - beta1) / 2.0; // note to self: better use biquad ?
    static float last_vals[2] = {0.0f};           // FIR filter buffer

    for (int i = 0; i < numSamples; i++)
    {
      float highFilteredSample;
      if (i < (numSamples - 1))
        highFilteredSample =
                beta1 * sampleBuffer[i] + beta2 * last_vals[0] + beta2 * sampleBuffer[i + 1]; // smooth out spikes
      else
        highFilteredSample = beta1 * sampleBuffer[i] + beta2 * last_vals[0] +
                             beta2 * last_vals[1]; // spcial handling for last sample in array
      last_vals[1] = last_vals[0];
      last_vals[0] = sampleBuffer[i];
      sampleBuffer[i] = highFilteredSample;
    }
  }

  // High-Pass filter, 6db per octave
  void runHighFilter6db(const float filter, uint16_t numSamples, float* sampleBuffer)
  {
    static float lowfilt = 0.0f; // IIR low frequency cutoff filter
    for (int i = 0; i < numSamples; i++)
    {
      lowfilt += filter * (sampleBuffer[i] - lowfilt); // lowpass
      sampleBuffer[i] = sampleBuffer[i] - lowfilt;     // lowpass --> highpass
    }
  }

  // High-Pass filter, 12db per octave
  void runHighFilter12db(const float filter, uint16_t numSamples, float* sampleBuffer)
  {
    static float lowfilt1 = 0.0f; // IIR low frequency cutoff filter - first pass = 6db
    static float lowfilt2 = 0.0f; // IIR low frequency cutoff filter - second pass = 12db
    for (int i = 0; i < numSamples; i++)
    {
      lowfilt1 += filter * (sampleBuffer[i] - lowfilt1); // first lowpass 6db
      // lowfilt2 += filter * (lowfilt1 - lowfilt2);     // second lowpass +6db
      // sampleBuffer[i] = sampleBuffer[i] - lowfilt2;   // lowpass --> highpass
      // implementation below has better results, compared to the code above
      float pass1Out = sampleBuffer[i] - lowfilt1; // output from first stage (lowpass --> highpass)
      lowfilt2 += filter * (pass1Out - lowfilt2);  // second lowpass +6db
      sampleBuffer[i] = pass1Out - lowfilt2;       // lowpass --> highpass
    }
  }

  // entry point of the program (TODO: protect it ?)
  float vReal[samplesFFT];
  float vImag[samplesFFT];

  // using latest ArduinoFFT lib, because it supports float and its much faster!
  // lib_deps += https://github.com/kosme/arduinoFFT#develop @ 1.9.2
  ArduinoFFT<float> FFT = ArduinoFFT<float>(vReal, vImag, samplesFFT, SAMPLE_RATE, windowWeighingFactors);

public:
  void reset()
  {
    FFT_Magnitude = 0;
    FFT_MajorPeak = 1;
    // reset FFT data
    fftCalc.fill(0);
    fftAvg.fill(0);
    fftResult.fill(0);
  }

  FftAnalyzer()
  {
    // fill the initial variables
    fill_frequency_bins();
    // reset variables
    reset();
  }

  float get_major_peak() { return FFT_MajorPeak; }

  float get_magnitude() { return FFT_Magnitude; }

  /**
   * \brief get the minimum frequency represented by the fft bin at index
   * \param[index] index of the fft bin
   * \return the min frequency of the bin
   */
  uint16_t get_bin_min_frequency(uint16_t index)
  {
    index = lmpd_constrain<uint16_t>(index, 0, fftResCount);
    return minFrequenciesPerBin[index];
  }

  /**
   * \brief get the minimum frequency represented by the fft bin at index
   * \param[index] index of the fft bin
   * \return the max frequency of the bin
   */
  uint16_t get_bin_max_frequency(uint16_t index)
  {
    index = lmpd_constrain<uint16_t>(index, 0, fftResCount);
    return maxFrequenciesPerBin[index];
  }

  uint8_t get_fft(uint16_t channel)
  {
    channel = lmpd_constrain<uint16_t>(channel, 0, fftResCount);
    return fftResult[channel];
  }

  void set_data(const int16_t data, uint16_t index)
  {
    index = lmpd_constrain<uint16_t>(index, 0, samplesFFT);
    vReal[index] = data;
    vImag[index] = 0;
  }

  // FFT main code
  void FFTcode()
  {
    // input filters applied before FFT
    if (useInputFilter > 0)
    {
      // filter parameter - we use constexpr as it does not need any RAM
      // (evaluted at compile time) value = 1 - exp(-2*c_PI * FFilter / FSample);
      // // FFilter: filter cutoff frequency; FSample: sampling frequency
      constexpr float filter30Hz = 0.01823938f;  // rumbling = 10-25hz
      constexpr float filter70Hz = 0.04204211f;  // mains hum = 50-60hz
      constexpr float filter120Hz = 0.07098564f; // bad microphones deliver noise below 120Hz
      constexpr float filter185Hz = 0.10730882f; // environmental noise is strongest below 180hz: wind,
                                                 // engine noise, ...
      switch (useInputFilter)
      {
        case 1:
          runMicFilter(samplesFFT, vReal);
          break; // PDM microphone bandpass
        case 2:
          runHighFilter12db(filter30Hz, samplesFFT, vReal);
          break; // rejects rumbling noise
        case 3:
          runMicSmoothing_v2(samplesFFT,
                             vReal); // slightly reduce high frequency noise and artefacts
          runHighFilter12db(filter70Hz, samplesFFT,
                            vReal); // rejects rumbling + mains hum
          break;
        case 4:
          runMicSmoothing_v2(samplesFFT,
                             vReal); // slightly reduce high frequency noise and artefacts
          runHighFilter6db(filter120Hz, samplesFFT,
                           vReal); // rejects everything below 110Hz
          break;
        case 5:
          runMicSmoothing(samplesFFT,
                          vReal); // reduce high frequency noise and artefacts
          runHighFilter6db(filter185Hz, samplesFFT,
                           vReal); // reject low frequency noise
          break;
      }
    }

    // find highest sample in the batch
    const int halfSamplesFFT = samplesFFT / 2; // samplesFFT divided by 2
    float maxSample1 = 0.0;                    // max sample from first half of FFT batch
    float maxSample2 = 0.0;                    // max sample from second half of FFT batch
    for (int i = 0; i < samplesFFT; i++)
    {
      // set imaginary parts to 0
      vImag[i] = 0;
      // pick our  our current mic sample - we take the max value from all
      // samples that go into FFT
      if ((vReal[i] <= (INT16_MAX - 1024)) &&
          (vReal[i] >= (INT16_MIN + 1024))) // skip extreme values - normally these are artefacts
      {
        if (i <= halfSamplesFFT)
        {
          if (fabsf(vReal[i]) > maxSample1)
            maxSample1 = fabsf(vReal[i]);
        }
        else
        {
          if (fabsf(vReal[i]) > maxSample2)
            maxSample2 = fabsf(vReal[i]);
        }
      }
    }

    FFT.dcRemoval(); // remove DC offset
    // FFT.windowing(FFTWindow::Flat_top, FFTDirection::Forward);  // Weigh data
    // using "Flat Top" window - better amplitude accuracy
    FFT.windowing(FFTWindow::Blackman_Harris,
                  FFTDirection::Forward); // Weigh data using "Blackman-
                                          // Harris" window - sharp peaks due
                                          // to excellent sideband rejection
    FFT.compute(FFTDirection::Forward);   // Compute FFT
    FFT.complexToMagnitude();             // Compute magnitudes
    //
    // vReal[3 .. 255] contain useful data, each a 20Hz interval (60Hz -
    // 5120Hz). There could be interesting data at bins 0 to 2, but there are
    // too many artifacts.
    //

    FFT.majorPeak(&FFT_MajorPeak,
                  &FFT_Magnitude); // let the effects know which freq was most dominant
    FFT_MajorPeak = lmpd_constrain<float>(FFT_MajorPeak, 1.0f,
                                          5120.0f); // restrict value to range expected by effects
    FFT_Magnitude = fabsf(FFT_Magnitude);

    for (int i = 0; i < samplesFFT; i++)
    { // Values for bins 0 and 1 are WAY too
      // large. Might as well start at 3.
      float t = 0.0;
      t = fabsf(vReal[i]); // just to be sure - values in fft bins should be
                           // positive any way
      t = t / 16.0f;       // Reduce magnitude. Want end result to be linear and
                           // ~4096 max.
      fftBin[i] = t;
    } // for()

    /* This FFT post processing is a DIY endeavour. What we really need is
     * someone with sound engineering expertise to do a great job here AND most
     * importantly, that the animations look GREAT as a result.
     */
    float startIndex = startValidBin;
    for (uint16_t i = 0; i < fftResCount; ++i)
    {
      // create the new index
      const float endIndex = startIndex * multiplier;
      fftCalc[i] = fftMedian(round(startIndex), round(endIndex));
      // swap the indices
      startIndex = endIndex;

      //   Noise supression of fftCalc bins using soundSquelch adjustment for
      //   different input types.
      fftCalc[i] = fftCalc[i] - (float)soundSquelch * compute_linear_noise(i) / 4.0 <= 0 ? 0 : fftCalc[i];

      // Manual linear adjustment of gain using sampleGain adjustment for
      // different input types.
      if (enableAutoGain)
        fftCalc[i] = fftCalc[i] * (float)sampleGain / 40.0 * inputLevel / 128.0 +
                     (float)fftCalc[i] / 16.0; // with inputLevel adjustment

      // Now, let's dump it all into fftResult. Need to do this, otherwise other
      // routines might grab fftResult values prematurely.
      // fftResult[i] = (int)fftCalc[i];
      fftResult[i] = lmpd_constrain<uint8_t>((int)fftCalc[i],
                                             0,
                                             254); // question: why do we constrain values to 8bit here ???
      fftAvg[i] = (float)fftResult[i] * .05 + (1 - .05) * fftAvg[i];
    }

    // Looking for fftResultMax for each bin using Pink Noise
    //      for (int i=0; i<fftResCount; i++) {
    //          fftResultMax[i] = ((fftResultMax[i] * 63.0) + fftResult[i])
    //          / 64.0;
    //         Serial.print(fftResultMax[i]*fftResultPink[i]);
    //         Serial.print("\t");
    //        }
    //      Serial.println(" ");
  } // FFTcode()
};

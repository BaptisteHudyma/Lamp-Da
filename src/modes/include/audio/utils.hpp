#ifndef MODES_INCLUDE_AUDIO_UTILS_HPP
#define MODES_INCLUDE_AUDIO_UTILS_HPP

/// @file utils.hpp

#include "src/system/ext/math8.h"
#include <cmath>
#include <cstdint>
#include <deque>
#include <string>

/// User modes audio utilities
namespace modes::audio {

/** \brief Sound processor able to detect sound level events
 *
 * For example, it can be used as follows:
 *
 * @code{.cpp}
 *
 *    static void on_enter_mode(auto& ctx) {
 *      ctx.soundEvent.reset(ctx);
 *    }
 *
 *    static void loop(auto& ctx) {
 *      auto& snd = ctx.soundEvent;
 *
 *      // update SoundEvent internal variables
 *      snd.update(ctx);
 *
 *      // ...
 *      // code that uses avgDelta / hasEvent / eventScale / etc
 *      // ...
 *    }
 *
 * @endcode
 *
 * Every time this class is updated, it will:
 *
 *  - measure the sound level using ``ctx.lamp.get_sound_level()``
 *  - update ``avgLevel`` a sound level "rolling" average
 *  - update ``avgMax`` a peak sound level "decaying" average
 *  - compute ``delta`` "contrast" between measured level and its average
 *  - update ``avgDelta`` an adjusted "rolling" average of that contrast
 *  - update ``hasEvent`` and ``eventScale`` accordingly
 *
 * You can use ``avgDelta`` as-is to build modes reactive to the surrounding
 * sounds while being able to somewhat ignore noisy environments.
 *
 * You can configure ``eventCutoff`` (in centibels) as the minimum sound
 * level required to trigger a ``delta`` contrast of 1.0 (the larger the
 * cut-off, the larger the required difference against the average level).
 *
 * The ``hasEvent`` boolean is triggered if ``avgDelta`` is above 1.0 for
 * at least ``nbEventSample`` ticks. The ``eventScale`` integer is
 * proportional to the largest ``avgMax * delta`` value during this period,
 * scaled down by ``eventNorm`` (in centibels).
 *
 * The ``windowSize`` (in quarter-ticks) is used for ``avgLevel`` and
 * ``avgMax`` (the larger the window, the less reactive the average) and
 * ``windowShort`` is used for ``avgDelta`` (shorter to be more reactive).
 *
 * Note that the algorithm implemented is simple and produces false positives.
 */
template<int eventCutoff = 500,
         int eventNorm = 500,
         int nbEventSample = 10,
         int windowSize = 1024,
         int windowShort = 64>
struct SoundEventTy
{
  /// Floor level for computations using ``AvgMax`` (lowest peak average)
  static constexpr float _eventCutoff = eventCutoff / 100.0;

  /// Inverse scaling factor for ``eventScale``
  static constexpr float _eventNorm = eventNorm / 100.0;

  /// Window size used for ``avgLevel`` and ``avgMax``
  static constexpr float _windowSize = windowSize / 4.0;

  /// Window size used for ``avgDelta``
  static constexpr float _windowShort = windowShort / 4.0;

  /// number of sample in a microphone run
  static constexpr size_t _dataLenght = microphone::SoundStruct::SAMPLE_SIZE;
  /// target value reached by the auto gain
  static constexpr int16_t _autoGainTargetValue = microphone::gainedSignalTarget;
  /// number of channels in the log fft
  static constexpr size_t _fftChannels = microphone::SoundStruct::numberOfFFtChanels;

  using FFTContainer = std::array<float, _dataLenght / 2>;
  using FFTLogContainer = std::array<float, _fftChannels>;
  // TODO: this should be a fixed size array
  using FFTHistoryContainer = std::deque<FFTLogContainer>;

  /// Call this once inside the mode on_enter_mode callback
  void reset(auto& ctx)
  {
    level = 10.0;
    avgLevel = level;
    avgMax = level;
    delta = 0;
    avgDelta = 0;
    hasEvent = false;
    eventScale = 0;
    _eventCount = 0;
    _eventScale = 0;

    data.fill(0);
    dataAutoGained.fill(0);
    fft_log.fill(0);
    fft_raw.fill(0);
  }

  /// Call this once every tick inside the mode loop callback
  void update(auto& ctx)
  {
    const microphone::SoundStruct& soundObject = ctx.lamp.get_sound_struct();

    // copy microphone data
    data = soundObject.data;
    dataAutoGained = soundObject.rectifiedData;
    fft_log = soundObject.fft_log;
    fft_raw = soundObject.fft_raw;
    fft_log_end_frequencies = soundObject.fft_log_end_frequencies;
    maxAmplitude = soundObject.maxAmplitude;
    maxAmplitudeFrequency = soundObject.maxAmplitudeFrequency;

    // average input sound over a second-long window (approx)
    const auto soundLevel = soundObject.sound_level_Db;
    level = (not std::isinf(soundLevel) and not std::isnan(soundLevel) and soundLevel > -70) ? soundLevel : -70;
    avgLevel = (level + avgLevel * _windowSize) / (_windowSize + 1);

    // average maximum over the same long window (approx)
    avgMax = (avgLevel + MAX(level, avgMax) * _windowSize) / (_windowSize + 1);
    avgMax = MAX(_eventCutoff, avgMax);

    // delta between instantaneous level & average, squashed by its max
    delta = MAX(level - avgLevel, 0) / avgMax;

    // average of delta square (1.0 split around _eventCutoff)
    avgDelta = (delta * delta + avgDelta * _windowShort) / (_windowShort + 1);

    // if avgDelta looks triggered, start sampling event
    if (avgDelta > 1.0)
    {
      _eventCount += 1;
      _eventScale = MAX(_eventScale, 256.0 * (avgMax / _eventNorm) * (1.0 + delta));

      // if no trigger, decay event
    }
    else
    {
      _eventCount = MAX(1, _eventCount - 1);
    }

    // if event is active, and event decayed, reset eventScale & hasEvent
    if (hasEvent)
    {
      _eventCount = 0;
      _eventScale = 0;
      eventScale = 0;
      hasEvent = false;
    }

    // if no event is active, and enough samples, set eventScale & hasEvent
    if (!hasEvent && _eventCount > nbEventSample)
    {
      eventScale = _eventScale;
      hasEvent = true;
    }

#if 0 // BE CAREFUL: RAM IS AT IT'S LIMIT WHEN USING THIS
      // add sample to history
    if (_FFTHistory_beatDetector.size() >= _FFThistory_MaxSize)
      _FFTHistory_beatDetector.pop_front();
    _FFTHistory_beatDetector.push_back(fft_log);

    // detect beats
    beatDetected = track_beat_events(fft_log, _FFTHistory_beatDetector, _FFThistory_MaxSize);
#endif
  }

  /// After the update, given a range, will return a boolean for beat detection
  bool is_beat_on_freq_range(const float minFreq, const float maxFreq)
  {
    size_t nbDataPoints = 0;
    size_t beatCnt = 0;

    size_t i = 0;
    // climb to min freq bin
    for (; i < fft_log_end_frequencies.size(); ++i)
    {
      const float maxFrequencyForBin = fft_log_end_frequencies[i];
      if (minFreq < maxFrequencyForBin)
        break;
    }
    // register beat events
    for (; i < fft_log_end_frequencies.size(); ++i)
    {
      nbDataPoints++;
      if (beatDetected[i])
        beatCnt++;

      const float maxFrequencyForBin = fft_log_end_frequencies[i];
      if (maxFrequencyForBin >= maxFreq)
        break;
    }
    return beatCnt > 0 && beatCnt >= ceil(0.5f * nbDataPoints);
  }

  float level = 10;    ///< Last sound level measured
  float avgLevel = 10; ///< Rolling sound level average
  float avgMax = 10;   ///< Decaying sound "peak" level average
  float delta = 0;     ///< Last sound "contrast" computed (centered around 1.0)
  float avgDelta = 0;  ///< Rolling sound "contrast" average (squared)
  bool hasEvent;       ///< Did an event happened last tick? (reset each loop)
  uint8_t eventScale;  ///< Event scale (0-255)
  float maxAmplitude;
  float maxAmplitudeFrequency;

  ///< frequency resolution of the raw fft result
  const float fftResolutionHz = microphone::SoundStruct::get_fft_resolution_Hz();
  const size_t _FFThistory_MaxSize = round(microphone::SoundStruct::get_fft_resolution_Hz());

  ///< raw microphone data
  std::array<int16_t, _dataLenght> data;
  ///< dynamically sound adjusted data
  std::array<int16_t, _dataLenght> dataAutoGained;
  ///< fast fourrier transform as a log scale (closer to human perception)
  FFTLogContainer fft_log;
  ///< set to true when the corresponding frequency range registers a beat
  std::array<bool, _fftChannels> beatDetected;
  ///< fast fourrier transform raw results
  std::array<float, _dataLenght / 2> fft_raw;

private:
  uint8_t _eventCount = 0;
  uint8_t _eventScale = 0;

  ///< store the history of the fft, on a few successiv samples
  FFTHistoryContainer _FFTHistory_beatDetector;
  std::array<float, _fftChannels> fft_log_end_frequencies;

  /// track the beat events using the FFT
  template<size_t T> static inline std::array<bool, T> track_beat_events(const std::array<float, T>& data,
                                                                         const FFTHistoryContainer& dataHistory,
                                                                         const size_t historySizeForBeatDetection)
  {
    std::array<bool, T> beats;
    beats.fill(false);

    // beat detection starts after a bit
    const size_t dataHistoryCnt = dataHistory.size();
    const float oneOverdataHistory = 1.0f / dataHistoryCnt;
    if (dataHistoryCnt == historySizeForBeatDetection)
    {
      std::array<float, T> averageFft;
      averageFft.fill(0.0f);
      for (const auto& fft: dataHistory)
      {
        for (size_t i = 0; i < T; i++)
          averageFft[i] += fft[i] * oneOverdataHistory;
      }

      std::array<float, T> varianceFft;
      varianceFft.fill(0.0f);
      for (const auto& fft: dataHistory)
      {
        for (size_t i = 0; i < T; i++)
        {
          const float val = fft[i] - averageFft[i];
          varianceFft[i] += val * val;
        }
      }

      for (size_t i = 0; i < T; i++)
      {
        varianceFft[i] *= oneOverdataHistory;
        const float stdDev = sqrtf(varianceFft[i]);
        static constexpr float N = 1.5f;
        // beat if > med + N * variance
        // N=1 : greater than 68.0% of values
        // N=2 : greater than 95.4% of values
        // N=3 : greater than 99.6% of values
        // N=4 : greater than 99.8% of values
        beats[i] = data[i] > (averageFft[i] + N * stdDev);
      }
    }
    return beats;
  }
};

} // namespace modes::audio

#endif

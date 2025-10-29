#ifndef MODES_INCLUDE_AUDIO_UTILS_HPP
#define MODES_INCLUDE_AUDIO_UTILS_HPP

/// @file utils.hpp

#include "src/system/ext/math8.h"

/// User modes audio utilities
namespace modes::audio {

/** \brief Sound processor able to detect sound level events
 *
 * For example, it can be used as follows:
 *
 * @code{.cpp}
 *
 *
 *    struct StateTy
 *    {
 *      modes::audio::SoundEventTy<> soundEvent;
 *    };
 *
 *    static void on_enter_mode(auto& ctx) {
 *      ctx.state.soundEvent.reset(ctx);
 *    }
 *
 *    static void loop(auto& ctx) {
 *      auto& snd = ctx.state.soundEvent;
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
  static constexpr int16_t autoGainTargetValue = microphone::gainedSignalTarget;

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
    fft.fill(0);
    strongestPeakMagnitude = 0.0f;
    fftMajorPeakFrequency_Hz = 0.0f;
  }

  /// Call this once every tick inside the mode loop callback
  void update(auto& ctx)
  {
    const microphone::SoundStruct& soundObject = ctx.lamp.get_sound_struct();

    // copy microphone data
    data = soundObject.data;
    dataAutoGained = soundObject.rectifiedData;
    fft = soundObject.fft;
    strongestPeakMagnitude = soundObject.strongestPeakMagnitude;
    fftMajorPeakFrequency_Hz = soundObject.fftMajorPeakFrequency_Hz;

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
  }

  float level = 10;    ///< Last sound level measured
  float avgLevel = 10; ///< Rolling sound level average
  float avgMax = 10;   ///< Decaying sound "peak" level average
  float delta = 0;     ///< Last sound "contrast" computed (centered around 1.0)
  float avgDelta = 0;  ///< Rolling sound "contrast" average (squared)
  bool hasEvent;       ///< Did an event happened last tick? (reset each loop)
  uint8_t eventScale;  ///< Event scale (0-255)

  std::array<int16_t, _dataLenght> data;           ///< raw microphone data
  std::array<int16_t, _dataLenght> dataAutoGained; ///< dynamically sound adjusted data
  std::array<float, microphone::SoundStruct::numberOfFFtChanels> fft;
  float strongestPeakMagnitude;   ///< strongest peak of the FFT
  float fftMajorPeakFrequency_Hz; ///< most proeminent frequency

private:
  uint8_t _eventCount = 0;
  uint8_t _eventScale = 0;
};

} // namespace modes::audio

#endif

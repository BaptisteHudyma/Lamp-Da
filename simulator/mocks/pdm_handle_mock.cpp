#include <cstdint>
#include <deque>
#include <memory>

#include "src/system/platform/pdm_handle.h"
#include "src/system/platform/time.h"

#include "src/system/utils/utils.h"
#include "src/system/utils/fft.h"

#include <SFML/Graphics/PrimitiveType.hpp>

#include <SFML/Audio/SoundRecorder.hpp>

#define PDM_HANDLE_CPP

// to hook microphone & measure levelDb
class LevelRecorder : public sf::SoundRecorder
{
  virtual bool onStart() { return true; }

  virtual bool onProcessSamples(const std::int16_t* samples, std::size_t sampleCount)
  {
    // safety to prevent data accumulation
    if (buffers.size() > 32)
      buffers.clear();

    const size_t samplePerIteration = sampleCount / lampda::platform::microphone::PdmData::SAMPLE_SIZE;
    const uint64_t newTime = lampda::platform::time_us();
    const uint64_t sampleDuration = (newTime - sampleTime_us) / samplePerIteration;
    sampleTime_us = newTime;

    for (size_t I = 0; I < samplePerIteration; I++)
    {
      lampda::platform::microphone::PdmData newData;
      newData.sampleTime_us = newTime + sampleDuration * I;
      newData.sampleDuration_us = sampleDuration;

      newData.sampleRead = 0;
      size_t i = 0;
      for (; i < lampda::platform::microphone::PdmData::SAMPLE_SIZE; i++)
      {
        size_t index = I * lampda::platform::microphone::PdmData::SAMPLE_SIZE + i;
        if (index >= sampleCount)
          break;

        newData.data[i] = samples[index];
      }
      newData.sampleRead = i;

      buffers.emplace_back(newData);
    }
    return true;
  }

  virtual void onStop() {}

public:
  // sound goes out slowly
  std::deque<::lampda::platform::microphone::PdmData> buffers;
  uint64_t sampleTime_us;

  ~LevelRecorder() { stop(); }
};
std::unique_ptr<LevelRecorder> recorder;

namespace lampda {

namespace platform {
namespace microphone {
namespace _private {

platform::microphone::PdmData get()
{
  // safety
  if (recorder->buffers.size() > 32)
    recorder->buffers.pop_back();

  if (recorder && recorder->buffers.size() > 0)
  {
    const auto buff = recorder->buffers.front();
    recorder->buffers.pop_front();
    return buff;
  }
  else
  {
    return {};
  };
}

bool start()
{
  fprintf(stderr, "mic started\n");
  fflush(stderr);

  if (!recorder)
    recorder = std::make_unique<LevelRecorder>();

  return recorder->start(utils::fft::SAMPLE_RATE);
}

void stop()
{
  if (recorder)
  {
    recorder->stop();
    recorder = nullptr;
  }
}

} // namespace _private
} // namespace microphone
} // namespace platform
} // namespace lampda

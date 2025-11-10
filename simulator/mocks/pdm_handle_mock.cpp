#include <memory>

#include "src/system/platform/pdm_handle.h"

#include "src/system/utils/utils.h"
#include "src/system/platform/time.h"

#include <SFML/Graphics/PrimitiveType.hpp>

#include <SFML/Audio/SoundRecorder.hpp>

#define PDM_HANDLE_CPP

// to hook microphone & measure levelDb
class LevelRecorder : public sf::SoundRecorder
{
  virtual bool onStart() { return true; }

  virtual bool onProcessSamples(const std::int16_t* samples, std::size_t sampleCount)
  {
    data.sampleRead = 0;
    data.sampleDuration_us = 0; // TODO issue #132
    const size_t readCnt = min<size_t>(sampleCount, microphone::PdmData::SAMPLE_SIZE);
    for (std::size_t i = 0; i < readCnt; i++)
    {
      data.data[i] = samples[i];
    }
    data.sampleTime_us = time_us();
    data.sampleRead = readCnt;
    return true;
  }

  virtual void onStop() {}

public:
  microphone::PdmData data;
  ~LevelRecorder() { stop(); }
};
std::unique_ptr<LevelRecorder> recorder;

namespace microphone {
namespace _private {

PdmData get()
{
  if (recorder)
  {
    return recorder->data;
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

  return recorder->start(16000);
}

void stop()
{
  if (recorder)
  {
    recorder->stop();
    recorder = nullptr;
  }
}

// TODO issue #132 (mock sound input parameters)
SoundStruct soundStruct;
SoundStruct process_fft(const PdmData& data) { return soundStruct; }

} // namespace _private
} // namespace microphone

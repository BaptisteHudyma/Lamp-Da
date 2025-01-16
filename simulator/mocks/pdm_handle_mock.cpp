#include "src/system/platform/pdm_handle.h"

#include "src/system/utils/utils.h"
#include "src/system/platform/time.h"

#include <cmath>
#include <iostream>

#include <SFML/Audio/SoundRecorder.hpp>

#define PDM_HANDLE_CPP

// to hook microphone & measure levelDb
class LevelRecorder : public sf::SoundRecorder
{
  virtual bool onStart() { return true; }

  virtual bool onProcessSamples(const sf::Int16* samples, std::size_t sampleCount)
  {
    data.sampleRead = 0;
    data.sampleDuration_us = 0; // TODO
    const size_t readCnt = min(sampleCount, microphone::PdmData::SAMPLE_SIZE);
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
LevelRecorder recorder;

namespace microphone {
namespace _private {

PdmData get() { return recorder.data; }

bool start() { return recorder.start(16000); }

void stop() { recorder.stop(); }

// TODO
SoundStruct soundStruct;
SoundStruct process_fft(const PdmData& data) { return soundStruct; }

} // namespace _private
} // namespace microphone

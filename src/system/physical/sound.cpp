#include "sound.h"

#include <cstdint>

#include "src/system/utils/utils.h"

#include "src/system/platform/time.h"
#include "src/system/platform/gpio.h"
#include "src/system/platform/print.h"

namespace microphone {

bool isStarted = false;
uint32_t lastMicFunctionCall = 0;

bool enable()
{
  lastMicFunctionCall = time_ms();
  if (isStarted)
    return true;

  DigitalPin(DigitalPin::GPIO::Output_EnableMicrophone).set_high(true);
  isStarted = _private::start();
  return isStarted;
}

void disable()
{
  if (!isStarted)
    return;

  _private::stop();
  DigitalPin(DigitalPin::GPIO::Output_EnableMicrophone).set_high(false);
  isStarted = false;
}

void disable_after_non_use()
{
  if (isStarted and (time_ms() - lastMicFunctionCall > 1000.0))
  {
    // disable microphone if last reading is old
    disable();
    lampda_print("mic stop: non use");
  }
}

float get_sound_level_Db(const PdmData& data)
{
  static float lastValue = 0;

  if (!data.is_valid())
    return lastValue;

  float sumOfAll = 0.0;
  const uint16_t samples = min<uint16_t>(PdmData::SAMPLE_SIZE, data.sampleRead);
  for (uint16_t i = 0; i < samples; i++)
  {
    sumOfAll += powf(data.data[i] / 1024.0f, 2.0f);
  }
  const float average = sumOfAll / static_cast<float>(samples);

  lastValue = 20.0f * log10f(sqrtf(average));
  // convert to decibels
  return lastValue;
}

float get_sound_level_Db()
{
  if (!enable())
  {
    // ERROR
    return 0.0;
  }
  return get_sound_level_Db(_private::get());
}

SoundStruct get_fft()
{
  if (!enable())
  {
    // ERROR
    return SoundStruct();
  }
  return _private::process_fft(_private::get());
}

} // namespace microphone

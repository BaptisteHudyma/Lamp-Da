#include "src/system/platform/MicroPhone.h"

#include "simulator.h"

#include "simulator/include/hardware_influencer.h"
#include <cmath>

#define MICROPHONE_IMPL_CPP

float _soundLevel = 0.0;
namespace mock_microphone {
void set_sound_level(float soundLevel) { _soundLevel = soundLevel; }
} // namespace mock_microphone

namespace microphone {

void enable() {}

void disable() {}

void disable_after_non_use() {}

float get_sound_level_Db() { return _soundLevel; }

static SoundStruct soundStruct;
SoundStruct get_fft() { return soundStruct; }

} // namespace microphone

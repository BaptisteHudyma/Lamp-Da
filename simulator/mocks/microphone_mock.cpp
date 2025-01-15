#include "src/system/platform/MicroPhone.h"

#include "simulator.h"

#include "simulator/include/hardware_influencer.h"
#include <cmath>

#define MICROPHONE_IMPL_CPP

namespace mock_microphone {
float soundLevel;
} // namespace mock_microphone

namespace microphone {

void enable() {}

void disable() {}

void disable_after_non_use() {}

float get_sound_level_Db() { return mock_microphone::soundLevel; }

static SoundStruct soundStruct;
SoundStruct get_fft() { return soundStruct; }

} // namespace microphone

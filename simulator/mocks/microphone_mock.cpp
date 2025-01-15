#include "src/system/platform/MicroPhone.h"

#include "simulator.h"

#include <cmath>

#define MICROPHONE_IMPL_CPP

namespace microphone {

void enable() {}

void disable() {}

void disable_after_non_use() {}

float get_sound_level_Db() { return 0.0; } // simulator<defaultSimulation>::recorder::_soundLevel; }

static SoundStruct soundStruct;
SoundStruct get_fft() { return soundStruct; }

} // namespace microphone

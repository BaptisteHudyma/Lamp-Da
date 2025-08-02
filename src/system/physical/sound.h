#ifndef PHYSICAL_SOUND_H
#define PHYSICAL_SOUND_H

#include "src/system/platform/pdm_handle.h"

namespace microphone {

// decibel level for a silent room
constexpr float silenceLevelDb = -57.0;
// microphone is not good enough at after this
constexpr float highLevelDb = 80.0;

bool enable();
void disable();

// disable microphone if last use is old
void disable_after_non_use();

/**
 * \return the average sound level in decibels
 */
float get_sound_level_Db();

/**
 * \brief compute the fast fourrier transform of the most recent sound sample
 * \return The frequency analysis object
 */
SoundStruct get_fft();

} // namespace microphone

#endif

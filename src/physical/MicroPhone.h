#ifndef MICRO_PHONE_H
#define MICRO_PHONE_H

#include <stdint.h>

namespace sound {

// decibel level for a silent room
constexpr float silenceLevelDb = -57.0;
constexpr float highLevelDb =
    80.0;  // microphone is not good enough at after this

// start the microphone readings
void enable_microphone();
// close the microphone readings
void disable_microphone();

/**
 * \return the average sound level in decibels
 */
float get_sound_level_Db();

}  // namespace sound

#endif
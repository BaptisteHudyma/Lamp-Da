#ifndef MICRO_PHONE_H
#define MICRO_PHONE_H

#include <stdint.h>

namespace microphone {

// decibel level for a silent room
constexpr float silenceLevelDb = -57.0;
constexpr float highLevelDb =
    80.0;  // microphone is not good enough at after this

// start the microphone readings
extern void enable();
// close the microphone readings
extern void disable();

/**
 * \return the average sound level in decibels
 */
extern float get_sound_level_Db();

}  // namespace microphone

#endif
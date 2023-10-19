#ifndef MICRO_PHONE_H
#define MICRO_PHONE_H

#include <stdint.h>
#include "animations.h"

namespace sound {

// decibel level for a silent room
const float silenceLevelDb = -57.0;
const float highLevelDb = 80.0; // microphone is not good enough at after this

// start the microphone readings
void enable_microphone(const uint32_t sampleRate);
// close the microphone readings
void disable_microphone();


/**
 * \return the average sound level in decibels
 */
float get_sound_level_Db();

/**
 * \brief Vu meter: should be reactive
 */
void vu_meter(const Color& vuColor, LedStrip& strip);

/**
 * \brief beat the color to the music pulse
 * \return true when a beat is detected
 */
bool pulse_beat_wipe(const Color& color, LedStrip& strip);

}

#endif
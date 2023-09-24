#ifndef MICRO_PHONE_H
#define MICRO_PHONE_H

#include <stdint.h>
#include "animations.h"

namespace sound {

// decibel level for a silent room
const float silenceLevelDb = -57.0;
const float highLevelDb = 80.0; // microphone is not good enough at after this

void init_microphone(const uint32_t sampleRate);

float get_beat_probability();


/**
 * \return the average sound level in decibels
 */
float get_sound_level_Db();

/**
 * \brief Vu meter: should be reactive
 */
void vu_meter(const Color& vuColor, Adafruit_NeoPixel& strip);

/**
 * \brief beat the color to the music pulse
 * \return true when a beat is detected
 */
bool pulse_beat_wipe(const Color& color, Adafruit_NeoPixel& strip);

}

#endif
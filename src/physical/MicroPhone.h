#ifndef MICRO_PHONE_H
#define MICRO_PHONE_H

#include <stdint.h>

#include "../colors/animations.h"

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

/**
 * \brief Vu meter: should be reactive
 */
void vu_meter(const Color& vuColor, LedStrip& strip);

void fftDisplay(const uint8_t speed, const uint8_t scale,
                const palette_t& palette, const bool reset, LedStrip& strip,
                const uint8_t nbBands = stripXCoordinates);

void mode_ripplepeak(const uint8_t rippleNumber, const palette_t& palette,
                     LedStrip& strip);
void mode_2DWaverly(const uint8_t speed, const uint8_t scale,
                    const palette_t& palette, LedStrip& strip);

}  // namespace sound

#endif
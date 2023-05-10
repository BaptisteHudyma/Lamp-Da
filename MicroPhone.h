#ifndef MICRO_PHONE_H
#define MICRO_PHONE_H

#include <stdint.h>


void init_microphone(const uint32_t sampleRate);

float get_beat_probability();

float get_vu_meter_level();

#endif
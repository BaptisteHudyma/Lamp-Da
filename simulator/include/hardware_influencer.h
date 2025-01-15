#ifndef HARDWARE_INFLUENCER_H
#define HARDWARE_INFLUENCER_H

namespace mock_gpios {
// update gpios callbacks
void update_callbacks();
} // namespace mock_gpios

namespace mock_registers {
extern bool isDeepSleep;
} // namespace mock_registers

namespace mock_microphone {
void set_sound_level(float soundLevel);
}

namespace mock_indicator {
uint32_t get_color();
}

#endif

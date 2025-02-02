#ifndef HARDWARE_INFLUENCER_H
#define HARDWARE_INFLUENCER_H

#include <cstdint>

namespace mock_gpios {
// update gpios callbacks
void update_callbacks();
} // namespace mock_gpios

namespace mock_registers {
extern bool isDeepSleep;
extern float cpuTemperature;
extern uint32_t addedAlgoDelay;
// run the other thread functions
extern bool shouldStopThreads;
void run_threads();
} // namespace mock_registers

namespace mock_indicator {
uint32_t get_color();
}

namespace mock_battery {
extern float voltage;
}

#endif

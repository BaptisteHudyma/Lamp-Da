#ifndef HARDWARE_INFLUENCER_H
#define HARDWARE_INFLUENCER_H

#include <cstdint>

namespace mock_gpios {
// update gpios callbacks
void update_callbacks();
} // namespace mock_gpios

namespace time_mocks {
void reset();
}

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

namespace mock_electrical {
// output at the power rail
extern float powerRailVoltage;
extern float powerRailCurrent;
// output at the led output
extern float outputVoltage;
extern float outputCurrent;
// output on vbus rail
extern float vbusVoltage;
extern float vbusCurrent;

// voltage applied to the USB input (controled by user)
extern float inputVbusVoltage;
// keep the OTG command voltage
extern float chargeOtgOutput;
} // namespace mock_electrical

namespace mock_battery {
extern float voltage;
}

#endif

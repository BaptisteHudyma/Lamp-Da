/*! \file hardware_influencer.h
    \brief Handle the physical simulation paremeters of a real lamp.
*/

#ifndef HARDWARE_INFLUENCER_H
#define HARDWARE_INFLUENCER_H

#include <cstdint>

/// Simulator dedicated namespace
namespace simulator {

/// Encapsulate the mock GPIO signals
namespace mock_gpios {
// update gpios callbacks
void update_callbacks();
} // namespace mock_gpios

/// Encapsulate the mock time signals
namespace time_mocks {
void reset();
}

/// Encapsulate the mock board registers signals
namespace mock_registers {
extern bool isDeepSleep;
extern float cpuTemperature;
extern uint32_t addedAlgoDelay;
// run the other thread functions
extern bool shouldStopThreads;
void run_threads();
} // namespace mock_registers

/// Encapsulate the mock indicator signals
namespace mock_indicator {
uint32_t get_color();
}

/// Encapsulate the mock electrical simulation signals
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

/// Encapsulate the mock battery signals
namespace mock_battery {
extern float voltage;
}

} // namespace simulator

#endif

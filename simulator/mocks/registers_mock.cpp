#include "src/system/platform/registers.h"

#include "simulator/include/hardware_influencer.h"
#include <cstdint>

#define PLATFORM_REGISTER_CPP

namespace mock_registers {
bool isDeepSleep = false;
float cpuTemperature;
uint32_t addedAlgoDelay;

bool shouldStopThreads = false;

} // namespace mock_registers

// setup the software watchedog
void setup_watchdog(const uint32_t timeoutDelaySecond) {}

void kick_watchdog(const uint8_t registerId) {}

// setup the ADC
void setup_adc(uint8_t resolution) {}

// get the number of wire com interfaces
uint8_t get_wire_interface_count() { return 2; }

// enter the dfu program mode (/!\ remove this program /!\)
void enter_serial_dfu() {}

// voltage is detected on VBUS line
bool is_voltage_detected_on_vbus() { return false; }

// microcontroler restarted after a reset
bool is_started_from_reset() { return false; }
// reset after a watchdog timeout
bool is_started_from_watchdog() { return false; }
// started by user interrupt
bool is_started_from_interrupt() { return true; }

float read_CPU_temperature_degreesC() { return mock_registers::cpuTemperature; }

void go_to_sleep(const int wakeUpPin) { mock_registers::isDeepSleep = true; };
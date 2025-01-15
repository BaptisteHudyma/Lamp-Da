#include "src/system/platform/registers.h"

#include "simulator/include/hardware_influencer.h"
#include <vector>

#define PLATFORM_REGISTER_CPP

typedef void (*taskfunc_t)(void);
std::vector<taskfunc_t> threadPool;

namespace mock_registers {
bool isDeepSleep = false;
float cpuTemperature;

bool shouldStopThreads = false;
void run_threads()
{
  // run until deep sleep
  while (not shouldStopThreads)
    for (auto& fun: threadPool)
    {
      fun();
    }
}
} // namespace mock_registers

// set tup the software watchedog
void setup_watchdog(const uint32_t timeoutDelaySecond) {}

void kick_watchdog() {}

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

void start_thread(taskfunc_t taskFunction) { threadPool.emplace_back(taskFunction); }
void yield_this_thread() {}

float read_CPU_temperature_degreesC() { return mock_registers::cpuTemperature; }

void go_to_sleep(int wakeUpPin) { mock_registers::isDeepSleep = true; };
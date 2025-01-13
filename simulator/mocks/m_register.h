// do not use pragma once here, has this can be mocked
#ifndef MOCK_REGISTERS_H
#define MOCK_REGISTERS_H

#define PLATFORM_REGISTER_CPP

#include "src/system/platform/registers.h"

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

typedef void (*taskfunc_t)(void);
void start_thread(taskfunc_t taskFunction) {}

#endif
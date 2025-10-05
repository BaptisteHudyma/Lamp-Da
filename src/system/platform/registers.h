// do not use pragma once here, has this can be mocked
#ifndef PLATFORM_REGISTERS_H
#define PLATFORM_REGISTERS_H

#include <cstdint>

// settup the software watchedog
extern void setup_watchdog(const uint32_t timeoutDelaySecond);
extern void kick_watchdog(const uint8_t registerId);

// get serial number 1 and 2 part
extern uint64_t get_device_serial_number();

// setup the ADC
extern void setup_adc(const uint8_t resolution);

// get the number of wire com interfaces
extern uint8_t get_wire_interface_count();

// enter the dfu program mode (/!\ remove this program /!\)
extern void enter_serial_dfu();

// voltage is detected on VBUS line
extern bool is_voltage_detected_on_vbus();

// microcontroler restarted after a reset
extern bool is_started_from_reset();
// reset after a watchdog timeout
extern bool is_started_from_watchdog();
// started by user interrupt
extern bool is_started_from_interrupt();

// make a read of the CPU temp
extern float read_CPU_temperature_degreesC();

// Put the system to sleep, with a wake up pin
extern void go_to_sleep(const int wakeUpPin);

#endif

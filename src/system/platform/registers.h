/*! \file registers.h
    \brief Interface for the platform specific microcontroler registers.
*/

// do not use pragma once here, has this can be mocked
#ifndef PLATFORM_REGISTERS_H
#define PLATFORM_REGISTERS_H

#include <cstdint>

/**
 * \brief Setup the software watchedog
 * \param[in] timeoutDelaySecond Delay after which the watchdog will reset the whole system.
 */
extern void setup_watchdog(const uint32_t timeoutDelaySecond);

/**
 * \brief Notify the software watchedog that we are alive
 * \param[in] registerId Watchdog index to kick
 */
extern void kick_watchdog(const uint8_t registerId);

/// get serial number
extern uint64_t get_device_serial_number();

/// setup the ADC
/// \param[in] resolution Power of 2 of the resolution (8, 10, 12 or 14).
extern void setup_adc(const uint8_t resolution);

/// get the number of wire com interfaces
extern uint8_t get_wire_interface_count();

/// enter the dfu program mode
/// \warning Remove the whole program, will need to flash again
extern void enter_serial_dfu();

/// voltage is detected on VBUS line
extern bool is_voltage_detected_on_vbus();

/// microcontroler restarted after a reset
extern bool is_started_from_reset();
/// reset after a watchdog timeout
extern bool is_started_from_watchdog();
/// started by user interrupt
extern bool is_started_from_interrupt();

/// make a read of the CPU temp
extern float read_CPU_temperature_degreesC();

/**
 * \brief Put the system to sleep, with a wake up pin
 * \param[in] wakeUpPin Pin id, defined by the system, that will be used to wake up again
 */
extern void go_to_sleep(const int wakeUpPin);

#endif

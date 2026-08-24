/*! \file serial.h
    \brief Interface for the platform specific serial input output.
*/

#ifndef HAL_SERIAL_H
#define HAL_SERIAL_H

#include <cstddef>
#include <cstdint>

namespace lampda {
namespace hal {
/// Serial port HAL layer
namespace serial {

/**
 * \brief call once at program start
 */
extern void init();

/// Return true if the serial port is active
extern bool is_activated();

/// Return true if a char is available to read
extern bool is_available();

/// Read a character (blocking)
extern char read();

/**
 * \brief Write a char buffer to the serial port
 * \param[in] buffer Buffer containing the text to display
 * \param[in] bufferSize Size of the buffer to display
 * \return the size of the written bytes
 */
extern size_t write(const char* const buffer, size_t bufferSize);

/// Return the usable MTU size
uint16_t mtu_size();

} // namespace serial
} // namespace hal
} // namespace lampda

#endif

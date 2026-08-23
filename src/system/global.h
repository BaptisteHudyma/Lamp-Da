/*! \file global.h
    \brief Main input point of the whole program
*/

#ifndef GLOBAL_H
#define GLOBAL_H

#include <cstdint>

/// Program scope
namespace lampda {

/// Main loop of the program
/// \param[in] addedDelay Debug tool: adds delay to the loop to test errors
extern void main_loop(const uint32_t addedDelay = 0);

/// Setup of the program, call once on systel start.
extern void main_setup();

// Document all main namespace of the project

// clang-format off

/// Hardware Abstraction Layer, handle the platform specific interactions. Can be changed to support different platforms
namespace hal {};
/// Board Support Layer, handle the low level logic above the HAL
namespace bsp {};
/// Medium level logic, above BSP.
namespace component {};
/// Handle the main high level logics
namespace logic {};

/// Specific component driver logic
namespace driver {};

/// Utility function and classes
namespace utils {};

// clang-format on

} // namespace lampda

#endif

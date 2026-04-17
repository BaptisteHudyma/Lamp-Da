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

/// Handle the main high level logics
namespace logic {};
/// Handle the physical modules drivers
namespace physical {};
/// Handle the platform specific interactions
namespace platform {};
/// Utility function and classes
namespace utils {};

// clang-format on

} // namespace lampda

#endif

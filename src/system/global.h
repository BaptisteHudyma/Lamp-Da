/*! \file global.h
    \brief Main input point of the whole program
*/

#ifndef GLOBAL_H
#define GLOBAL_H

#include <cstdint>

// global program scope
namespace global {

/// Main loop of the program
/// \param[in] addedDelay Debug tool: adds delay to the loop to test errors
extern void main_loop(const uint32_t addedDelay = 0);

/// Setup of the program, call once on systel start.
extern void main_setup();

} // namespace global

#endif

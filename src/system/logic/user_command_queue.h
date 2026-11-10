/*! \file user_command_queue.h
    \brief Run the user commands and user callbacks of other threads on the main loop.

    The user code (modes, lamp object, strip buffers, brightness callbacks) is not thread safe and must only run on
    the main loop. Threads other than the main loop (sun timer task, bluetooth callback task, ...) must not call
    user::handle_user_command() or user::brightness_update() directly: they post a request here, and the main loop
    runs it on its next tick.
*/

#ifndef LOGIC_USER_COMMAND_QUEUE_H
#define LOGIC_USER_COMMAND_QUEUE_H

#include <cstddef>

#include "src/system/common/user_commands.h"

namespace lampda {
namespace logic {
/// Defer the user commands to the main loop
namespace user_command_queue {

/// Maximum number of commands waiting for the main loop
static constexpr size_t maxStoredCommands = 16;

/**
 * \brief Post a user command, run by the main loop on its next tick. Thread safe
 * \param[in] command The command to run
 * \return false if the queue is full and the command was dropped
 */
bool post(const common::UserCommand& command);

/**
 * \brief Request a call to the user brightness callback (see brightness::force_brightness_user_callback()), run by the
 * main loop on its next tick, after the posted commands. Thread safe.
 * Several requests before the next tick result in a single call.
 */
void request_brightness_user_callback();

/// Return the number of commands waiting for the main loop. Thread safe
size_t get_pending_count();

/// Main loop only: run the posted commands, in order, then the requested brightness callback
void loop();

} // namespace user_command_queue
} // namespace logic
} // namespace lampda

#endif

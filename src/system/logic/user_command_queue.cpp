#include "user_command_queue.h"

#include "src/system/bsp/text_out.h"

#include "src/system/logic/brightness_handle.h"

#include "src/system/utils/thread_safe_queue.h"

#include "src/user/functions.h"

namespace lampda {
namespace logic {
namespace user_command_queue {

namespace {

/// Filled by any thread, emptied by the main loop
utils::ThreadSafeQueue<common::UserCommand, maxStoredCommands> commandQueue;

/// Set by any thread, cleared by the main loop
bool isBrightnessCallbackRequested = false;

} // namespace

bool post(const common::UserCommand& command)
{
  if (commandQueue.push(command))
    return true;

  bsp::lampda_print("User command queue full, command %d dropped", static_cast<int>(command.get_type()));
  return false;
}

void request_brightness_user_callback() { __atomic_store_n(&isBrightnessCallbackRequested, true, __ATOMIC_SEQ_CST); }

size_t get_pending_count() { return commandQueue.size(); }

void loop()
{
  // only run the commands present on entry: a command posting another one cannot keep the loop busy
  for (size_t count = commandQueue.size(); count > 0; --count)
  {
    const auto command = commandQueue.pop();
    if (not command.has_value())
      break;
    user::handle_user_command(command.value());
  }

  // after the commands: a sun timer posts its progress before requesting the brightness update
  if (__atomic_exchange_n(&isBrightnessCallbackRequested, false, __ATOMIC_SEQ_CST))
    brightness::force_brightness_user_callback();
}

} // namespace user_command_queue
} // namespace logic
} // namespace lampda

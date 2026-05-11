#include "inputs_bluetooth.h"

#include "src/user/functions.h"

#include "src/system/logic/behavior.h"
#include "src/system/logic/brightness_handle.h"
#include "src/system/logic/sunset_timer.h"

#include "src/system/platform/print.h"

#include "src/system/utils/utils.h"
#include <cstdint>

namespace lampda {
namespace logic {
namespace inputs_bluetooth {

/// keep track of the bluetooth uses
inline static bool _wasBluetoothUsed = false;

bool is_bluetooth_used() { return _wasBluetoothUsed; }

void handle_BLE_ELK_command(const utils::ELK::Package& elkControlCommand)
{
  _wasBluetoothUsed = true;

  lampda::user::handle_elk_command(elkControlCommand);
}

} // namespace inputs_bluetooth
} // namespace logic
} // namespace lampda

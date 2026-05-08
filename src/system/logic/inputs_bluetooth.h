#ifndef LOGIC_INPUTS_BLUETOOTH_H
#define LOGIC_INPUTS_BLUETOOTH_H

#include "src/system/utils/elk_decoder.h"

namespace lampda {
namespace logic {
/// Handle the system input, the behavior associated to the bluetooth inputs.
namespace inputs_bluetooth {

/**
 * \brief Handle an ELK bluetooth command
 */
void handle_BLE_ELK_command(const utils::ELK::Package& elkControlCommand);

} // namespace inputs_bluetooth
} // namespace logic
} // namespace lampda

#endif

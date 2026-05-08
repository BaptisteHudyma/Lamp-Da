/*! \file inputs_bluetooth.h
    \brief Handle the logic associated with the bluetooth inputs.
*/

#ifndef LOGIC_INPUTS_BLUETOOTH_H
#define LOGIC_INPUTS_BLUETOOTH_H

#include "src/system/utils/elk_decoder.h"

namespace lampda {
namespace logic {
/// Handle the system input, the behavior associated to the bluetooth inputs.
namespace inputs_bluetooth {

/// Indicates when called if a bluetooth command was received during the last power on
bool is_bluetooth_used();

/**
 * \brief Handle an ELK bluetooth command
 * \param[in] elkControlCommand Control command, assumed to be received by bluetooth
 */
void handle_BLE_ELK_command(const utils::ELK::Package& elkControlCommand);

} // namespace inputs_bluetooth
} // namespace logic
} // namespace lampda

#endif

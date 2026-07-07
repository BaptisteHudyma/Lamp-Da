/*! \file inputs.h
    \brief Handle the logic associated with the input button.
*/

#ifndef INPUTS_H
#define INPUTS_H

#include <cstdint>
#include "src/system/utils/queue.h"

namespace lampda {
namespace logic {
/// Handle the system input, the behavior associated to the button inputs.
namespace inputs {

/// private handling namespace, used for unit testing
namespace __private {

struct ButtonEvent
{
  bool isStartClick = false;  ///< indicates if this click is the first of the system
  bool isLongPress = false;   ///< indicates if this is a long press click
  uint32_t longPressDuration; ///< during a long press click, this is the press time. It goes to zero on release
  uint32_t clickCount;        ///< keep track of the number of clicks of the chain
};

static constexpr size_t maxButtonEventStore =
        15; ///< this event count should be high enough to not miss clicks and ramp events

extern utils::Queue<ButtonEvent, maxButtonEventStore> buttonEventQueue; ///< button event asynchroneous queue

} // namespace __private

/// Call once on system start
/// \param[in] wasPoweredByUserInterrupt Indicates if the system was powered by the user button interrupt
void init(const bool wasPoweredByUserInterrupt);

/// Call often to handle button updates
void loop();

/// disable custom user modes
void button_disable_usermode();

/// return true if custom user modes are enabled
bool is_button_usermode_enabled();

/// Signal a button click event
bool add_button_click_event(uint32_t clickCount, bool isStartClick);
/// Signal a button press event
bool add_button_press_event(uint32_t clickCount, uint32_t pressDuration, bool isStartClick);

} // namespace inputs
} // namespace logic
} // namespace lampda

#endif

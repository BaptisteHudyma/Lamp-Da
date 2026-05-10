/*! \file bleutooth_group.hpp
    \brief Implementation of the standard bleutooth group
*/

#ifndef LEGACY_BLEUTOOTH_GROUP_H
#define LEGACY_BLEUTOOTH_GROUP_H

#include "src/modes/bluetooth/control_fixed_modes.hpp"

namespace lampda::modes {
/// Special bluetooth modes
namespace bluetooth {

// Order is important here :
// Every index should correspond to the ELK standard index + 1
//  0: Special controled display mode
//  1: Static Red
//  2: Static Blue
//  3: Static Green
//  4: Static Cyan
//  5: Static Yellow
//  6: Static Purple
//  7: Static White
//  8: Three Color Jumping Change
//  9: Seven Color Jumping Change
// 10: Three Color Cross Fade
// 11: Seven Color Cross Fade
// 12: Red Gradual Change
// 13: Green Gradual Change
// 14: Blue Gradual Change
// 15: Yellow Gradual Change
// 16: Cyan Gradual Change
// 17: Purple Gradual Change
// 18: White Gradual Change
// 19: Red Green Cross Fade
// 20: Red Blue Cross Fade
// 21: Green Blue Cross Fade
// 22: Seven color Strobe Flash
// 23: Red Strobe Flash
// 24: Green Strobe Flash
// 25: Blue Strobe Flash
// 26: Yellow Strobe Flash
// 27: Cyan Strobe Flash
// 28: Purple Strobe Flash
// 29: White Strobe Flash

using BluetoothModes =
        modes::GroupFor<modes::bluetooth::ColorControlMode,                              // custom color controller
                        modes::bluetooth::FixedColorMode<colors::HTMLColorCode::Red>,    // 1: fixed Red color
                        modes::bluetooth::FixedColorMode<colors::HTMLColorCode::Blue>,   // 2: fixed Blue color
                        modes::bluetooth::FixedColorMode<colors::HTMLColorCode::Lime>,   // 3: fixed Green color
                        modes::bluetooth::FixedColorMode<colors::HTMLColorCode::Cyan>,   // 4: fixed Cyan color
                        modes::bluetooth::FixedColorMode<colors::HTMLColorCode::Yellow>, // 5: fixed Yellow color
                        modes::bluetooth::FixedColorMode<colors::HTMLColorCode::Purple>, // 6: fixed Purple color
                        modes::bluetooth::FixedColorMode<colors::HTMLColorCode::White>   // 7: fixed White color
                        >;

} // namespace bluetooth
} // namespace lampda::modes
#endif

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

using BluetoothModes = modes::GroupFor<modes::bluetooth::ColorControlMode,          // controlable color
                                       modes::bluetooth::FixedColorMode<255, 0, 0>, // fixed red color
                                       modes::bluetooth::FixedColorMode<0, 0, 255>, // fixed blue color
                                       modes::bluetooth::FixedColorMode<0, 255, 0>  // fixed green color
                                       >;

} // namespace bluetooth
} // namespace lampda::modes
#endif

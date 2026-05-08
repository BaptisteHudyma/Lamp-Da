#ifndef MODES_BLUETOOTH_CONTROLFIXEDMODES_H
#define MODES_BLUETOOTH_CONTROLFIXEDMODES_H

/// @file control_fixed_modes.hpp

#include "src/system/ext/math8.h"
#include "src/system/ext/noise.h"

#include "src/modes/include/colors/palettes.hpp"

namespace lampda::modes {
/// Define bluetooth special modes
namespace bluetooth {

struct ColorControlMode : public BasicMode
{
  struct StateTy
  {
    // color to display
    uint32_t color = 0xFFFFFF;
  };

  static void loop(auto& ctx)
  {
    //
    ctx.lamp.fill(ctx.state.color);
  }
};

template<modes::colors::HTMLColorCode color> struct FixedColorMode : public BasicMode
{
  static void loop(auto& ctx) { ctx.lamp.fill(color); }
};

} // namespace bluetooth
} // namespace lampda::modes

#endif

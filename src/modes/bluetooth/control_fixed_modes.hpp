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
    uint8_t red;
    uint8_t green;
    uint8_t blue;
  };

  static void loop(auto& ctx)
  {
    //
    uint32_t color = 0;
    color = ctx.state.red << 16 | ctx.state.green << 8 | ctx.state.blue;
    ctx.lamp.fill(color);
  }
};

template<uint8_t red, uint8_t green, uint8_t blue> struct FixedColorMode : public BasicMode
{
  static void loop(auto& ctx)
  {
    //
    const uint32_t color = red << 16 | green << 8 | blue;
    ctx.lamp.fill(color);
  }
};

} // namespace bluetooth
} // namespace lampda::modes

#endif

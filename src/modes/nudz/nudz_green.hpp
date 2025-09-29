
#include "heineken_image.hpp"

namespace modes::nudz
{

  struct NudzGreenMode : public BasicMode {

    static void loop(auto& ctx) {
//       ctx.lamp.fill(colors::Green);
      // ctx.lamp.setBrightness(ctx.lamp.tick % ctx.lamp.maxBrightness);
//       ctx.lamp.setPixelColorXY(1, 1, colors::Red);
      unsigned decal = unsigned(ctx.lamp.tick / 5);
      unsigned w = 25, h = 22;
      for(unsigned y=0; y<h; ++y)
        for(unsigned x=0; x<w; ++x)
          ctx.lamp.setPixelColorXY((x + decal) % w, y, heineken_image[y * w + x]);
    }

  };

}

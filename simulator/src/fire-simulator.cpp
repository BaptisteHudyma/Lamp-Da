#include <simulator.h>

#include "default_simulation.h"

void fire(const uint8_t scalex, const uint8_t scaley, const uint8_t speed,
          const palette_t& palette, LedStrip& strip) {
  static uint32_t step = 0;
  uint16_t zSpeed = step / (256 - speed);
  uint16_t ySpeed = millis() / (256 - speed);
  for (int i = 0; i < ceil(stripXCoordinates); i++) {
    for (int j = 0; j < ceil(stripYCoordinates); j++) {
      strip.setPixelColorXY(
          stripXCoordinates - i, j,
          get_color_from_palette(
              qsub8(noise8::inoise(i * scalex, j * scaley + ySpeed, zSpeed),
                    abs8(j - (stripXCoordinates - 1)) * 255 /
                        (stripXCoordinates - 1)),
              palette));
    }
  }
  step++;
}

struct fireSimulation : public defaultSimulation {
  float fps = 20.f;

  void loop(LedStrip& strip) {
    fire(60, 60, 255, PaletteHeatColors, strip);
  }

};

int main() {
  return simulator<fireSimulation>::run();
}


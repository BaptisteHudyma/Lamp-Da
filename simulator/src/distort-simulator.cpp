#include <simulator.h>

#include "default_simulation.h"

void mode_2Ddistortionwaves(const uint8_t scale, const uint8_t speed, LedStrip& strip)
{
  const uint16_t cols = ceil(stripXCoordinates);
  const uint16_t rows = ceil(stripYCoordinates);

  uint8_t _speed = speed / 32;
  uint8_t _scale = scale / 32;

  uint8_t w = 2;

  uint16_t a = millis() / 32;
  uint16_t a2 = a / 2;
  uint16_t a3 = a / 3;

  uint16_t cx = beatsin8(10 - _speed, 0, cols - 1) * _scale;
  uint16_t cy = beatsin8(12 - _speed, 0, rows - 1) * _scale;
  uint16_t cx1 = beatsin8(13 - _speed, 0, cols - 1) * _scale;
  uint16_t cy1 = beatsin8(15 - _speed, 0, rows - 1) * _scale;
  uint16_t cx2 = beatsin8(17 - _speed, 0, cols - 1) * _scale;
  uint16_t cy2 = beatsin8(14 - _speed, 0, rows - 1) * _scale;

  uint16_t xoffs = 0;
  for (int x = 0; x < cols; x++)
  {
    xoffs += _scale;
    uint16_t yoffs = 0;

    for (int y = 0; y < rows; y++)
    {
      yoffs += _scale;

      byte rdistort = cos8((cos8(((x << 3) + a) & 255) + cos8(((y << 3) - a2) & 255) + a3) & 255) >> 1;
      byte gdistort = cos8((cos8(((x << 3) - a2) & 255) + cos8(((y << 3) + a3) & 255) + a + 32) & 255) >> 1;
      byte bdistort = cos8((cos8(((x << 3) + a3) & 255) + cos8(((y << 3) - a) & 255) + a2 + 64) & 255) >> 1;

      COLOR c;
      c.red = rdistort + w * (a - (((xoffs - cx) * (xoffs - cx) + (yoffs - cy) * (yoffs - cy)) >> 7));
      c.green = gdistort + w * (a2 - (((xoffs - cx1) * (xoffs - cx1) + (yoffs - cy1) * (yoffs - cy1)) >> 7));
      c.blue = bdistort + w * (a3 - (((xoffs - cx2) * (xoffs - cx2) + (yoffs - cy2) * (yoffs - cy2)) >> 7));

      c.red = utils::gamma8(cos8(c.red));
      c.green = utils::gamma8(cos8(c.green));
      c.blue = utils::gamma8(cos8(c.blue));

      strip.setPixelColorXY(x, y, c);
    }
  }
}

struct distortSimulation : public defaultSimulation
{
  float fps = 20.f;

  void loop(LampTy& lamp) { mode_2Ddistortionwaves(128, 128, lamp.getLegacyStrip()); }
};

int main() { return simulator<distortSimulation>::run(); }

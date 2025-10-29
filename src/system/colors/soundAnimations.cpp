#ifdef LMBD_LAMP_TYPE__INDEXABLE

#include "soundAnimations.h"

#include "src/system/colors/animations.h"
#include "src/system/utils/constants.h"
#include "src/system/utils/coordinates.h"

namespace animations {

void fft_display(const uint8_t speed, const uint8_t scale, const palette_t& palette, LedStrip& strip)
{
  const auto& soundCharacs = microphone::get_sound_characteristics();

  static uint32_t lastCall = 0;
  static uint32_t call = 0;

  if (call == 0)
  {
    lastCall = 0;
    call = 0;
  }

  static const uint16_t cols = ceil(stripXCoordinates);
  static const uint16_t rows = ceil(stripYCoordinates);

  for (uint8_t x = 0; x < cols; ++x)
  {
    const uint8_t mappedX = lmpd_map<uint8_t>(x, 0, cols, 0, microphone::SoundStruct::numberOfFFtChanels - 1);
    const uint8_t mappedY = lmpd_map<uint8_t>(soundCharacs.fft[mappedX], 0, 255, 2, rows);
    for (uint8_t y = 0; y <= mappedY; y++)
    {
      uint8_t colorIndex = lmpd_map<uint8_t>(y, 0, rows - 1, 0, 255);

      const uint32_t ledColor = get_color_from_palette(colorIndex, palette);
      strip.setPixelColorXY(x, rows - y, ledColor);
    }
    for (uint8_t y = mappedY; y <= rows; ++y)
      strip.setPixelColorXY(x, rows - y, 0);
  }
}

} // namespace animations

#endif

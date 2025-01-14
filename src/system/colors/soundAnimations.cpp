#ifdef LMBD_LAMP_TYPE__INDEXABLE

#include "soundAnimations.h"

#include "src/system/colors/animations.h"
#include "src/system/utils/constants.h"
#include "src/system/utils/coordinates.h"

namespace animations {

void vu_meter(const Color& vuColor, const uint8_t fadeOut, LedStrip& strip)
{
  const float decibels = microphone::get_sound_level_Db();
  // convert to 0 - 1
  const float vuLevel = (decibels + abs(microphone::silenceLevelDb)) / microphone::highLevelDb;

  // display the gradient
  strip.fadeToBlackBy(fadeOut);
  animations::fill(vuColor, strip, vuLevel);
}

void fft_display(const uint8_t speed, const uint8_t scale, const palette_t& palette, LedStrip& strip)
{
  const auto& fftRes = microphone::get_fft();

  static uint32_t lastCall = 0;
  static uint32_t call = 0;
  static uint16_t* previousBarHeight = strip._buffer16b;

  if (call == 0)
  {
    lastCall = 0;
    call = 0;

    memset(previousBarHeight, 0, sizeof(previousBarHeight));
  }

  static const uint16_t cols = ceil(stripXCoordinates);
  static const uint16_t rows = ceil(stripYCoordinates);

  int fadeoutDelay = (256 - speed) / 64;
  if ((fadeoutDelay <= 1) || ((call % fadeoutDelay) == 0))
    strip.fadeToBlackBy(speed);

  // do not run if no data
  if (not fftRes.isValid)
    return;

  bool rippleTime = false;
  if (time_ms() - lastCall >= (256U - scale))
  {
    lastCall = time_ms();
    rippleTime = true;
  }

  for (uint8_t x = 0; x < cols; ++x)
  {
    const uint8_t mappedX = utils::map(x, 0, cols, 0, microphone::numberOfFFtChanels - 1);
    const uint8_t mappedY = utils::map(fftRes.fft[mappedX], 0, 255, 0, rows);

    if (mappedY > previousBarHeight[x])
      previousBarHeight[x] = mappedY; // drive the peak up

    uint32_t ledColor = 0; // black
    for (int y = 0; y < mappedY; y++)
    {
      uint8_t colorIndex = utils::map(y, 0, rows - 1, 0, 255);

      ledColor = get_color_from_palette(colorIndex, palette);
      strip.setPixelColorXY(x, rows - y, ledColor);
    }

    if (rippleTime && previousBarHeight[x] > 0)
      previousBarHeight[x]--; // delay/ripple effect
  }
}

} // namespace animations

#endif
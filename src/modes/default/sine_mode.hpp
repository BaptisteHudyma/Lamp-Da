#ifndef SINE_MODE_H
#define SINE_MODE_H

#include "src/modes/include/colors/utils.hpp"
#include "src/modes/include/colors/palettes.hpp"
namespace modes::default_modes {
struct SineMode2 : public modes::BasicMode
{
  static void loop(auto& ctx)
  {

    uint8_t speed = 128;
    uint8_t intensity = 128;
     
    static uint16_t step = 0;
    uint16_t colorIndex = time_ms() / 32; //(256 - SEGMENT.fft1);  // Amount of colour change.

    step += speed / 16;            // Speed of animation.
    uint16_t freq = intensity / 4; // SEGMENT.fft2/8;                       // Frequency of the signal.

    for (int i = 0; i < LED_COUNT; i++)
    {                                                 // For each of the LED's in the strand, set a brightness based on
                                                      // a wave as follows:
      uint8_t pixBri = cubicwave8((i * freq) + step); // qsuba(cubicwave8((i*freq)+SEGENV.step),
                                                      // (255-SEGMENT.intensity)); // qsub sets a minimum value
                                                      // called thiscutoff. If < thiscutoff, then bright = 0.
                                                      // Otherwise, bright = 128 (as defined in qsub)..
      // setPixCol(i, i*colorIndex/255, pixBri);
      COLOR pixColor;
      pixColor.color = modes::colors::from_palette((uint8_t)(i * colorIndex / 255), PaletteRainbowColors);
      COLOR back;
      back.color = 0;
      //ctx.lamp.setPixelColor(i, modes::colors::blend(back, pixColor, pixBri));
      ctx.lamp.setPixelColor(i, modes::colors::fade<false>(pixColor, pixBri));
	  }
  }
};
}
#endif
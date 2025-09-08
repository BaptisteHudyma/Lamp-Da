#ifndef PERLIN_NOISE_MODE_H
#define PERLIN_NOISE_MODE_H

/// @file perlin_noise.hpp

#include "src/system/ext/math8.h"
#include "src/system/ext/noise.h"
#include "src/system/ext/random8.h"

#include "src/modes/include/colors/palettes.hpp"

/// Basic "default" modes included with the hardware
namespace modes::default_modes {

struct PerlinNoiseMode : public BasicMode
{
  // hint manager to save our custom ramp
  static constexpr bool hasCustomRamp = true;

  // index of the buffer used in this mode
  static constexpr uint8_t bufferIndexToUse = 0;

  struct StateTy
  {
    float positionX;
    float positionY;
    float positionZ;

    float speed;
    uint16_t scale;

    uint16_t ihue;

    // store references to palettes
    const palette_t* _palettes[3] = {&PaletteLavaColors, &PaletteForestColors, &PaletteOceanColors};
    const uint8_t maxPalettesCount = 3;

    // store selected palette
    palette_t const* selectedPalette;
  };

  static void on_enter_mode(auto& ctx)
  {
    ctx.state.positionX = random16();
    ctx.state.positionY = random16();
    ctx.state.positionZ = random16();

    ctx.state.speed = 1000;
    ctx.state.scale = 600;

    ctx.state.ihue = 0;

    ctx.template set_config_bool<ConfigKeys::rampSaturates>(false);

    ctx.lamp.template fillTempBuffer<bufferIndexToUse>(0);

    custom_ramp_update(ctx, ctx.get_active_custom_ramp());
  }

  static void custom_ramp_update(auto& ctx, uint8_t rampValue)
  {
    auto& state = ctx.state;

    const uint8_t rampIndex = min(floor(rampValue / 255.0f * state.maxPalettesCount), state.maxPalettesCount - 1);
    state.selectedPalette = state._palettes[rampIndex];
  }

  static void loop(auto& ctx)
  {
    if (!ctx.state.selectedPalette)
      return;

    // reference to a value buffer
    static auto noiseBuffer = ctx.lamp.template getTempBuffer<bufferIndexToUse>();

    auto& state = ctx.state;

    // copy to prevent a ramp update mid animation
    const palette_t palette = *(state.selectedPalette);

    const auto x = state.positionX;
    const auto y = state.positionY;
    const auto z = state.positionZ;
    const auto scale = state.scale;

    static constexpr size_t everyNIndex = 2;
    const size_t firstIndex = ctx.lamp.tick % everyNIndex;

    // how much the new value changes the last one
    static constexpr float dataSmoothing = 0.01f;
    for (size_t i = firstIndex; i < LED_COUNT; i += everyNIndex)
    {
      // TODO: use new strip interface coordinates
      const auto res = ctx.lamp.getLegacyStrip().get_lamp_coordinates(i);
      uint16_t data = noise16::inoise(x + scale * res.x, y + scale * res.y, z + scale * res.z);

      // smooth over time to prevent suddent jumps
      noiseBuffer[i] = noiseBuffer[i] * dataSmoothing + data * (1.0 - dataSmoothing);
    }

    // apply slow drift to X and Y, just for visual variation.
    state.positionX += state.speed / 8;
    state.positionY -= state.speed / 16;

    for (size_t i = firstIndex; i < LED_COUNT; i += everyNIndex)
    {
      uint16_t index = noiseBuffer[i];
      uint8_t bri = noiseBuffer[LED_COUNT - 1 - i] >> 8;

      // if this palette is a 'loop', add a slowly-changing base value
      index += state.ihue;

      // brighten up, as the color palette itself often contains the
      // light/dark dynamic range desired
      if (bri > 127)
      {
        bri = 255;
      }
      else
      {
        bri = dim8_raw(bri * 2);
      }

      ctx.lamp.setPixelColor(i, colors::from_palette(index, palette, bri));
    }

    state.ihue += 1;
  }
};

} // namespace modes::default_modes

#endif

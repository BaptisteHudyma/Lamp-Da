#ifndef PERLIN_NOISE_MODE_H
#define PERLIN_NOISE_MODE_H

/// @file perlin_noise.hpp

#include "src/system/ext/math8.h"
#include "src/system/ext/noise.h"
#include "src/system/ext/random8.h"

#include "src/modes/include/colors/palettes.hpp"
#include <cstdint>
#include <cstdlib>

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
    uint32_t positionX;
    uint32_t positionY;
    uint32_t positionZ;

    int16_t speedX;
    int16_t speedY;
    int16_t speedZ;
    static constexpr uint16_t minSpeed = 25;
    static constexpr uint16_t maxSpeed = 400;

    uint16_t scale;

    uint16_t ihue;

    // store references to palettes
    static constexpr uint8_t maxPalettesCount = 4;
    const palette_t* _palettes[maxPalettesCount] = {&colors::PaletteRainbowColors,
                                                    &colors::PaletteLavaColors,
                                                    &colors::PaletteForestColors,
                                                    &colors::PaletteOceanColors};

    // store selected palette
    palette_t const* selectedPalette;
  };

  static void on_enter_mode(auto& ctx)
  {
    ctx.state.positionX = random16();
    ctx.state.positionY = random16();
    ctx.state.positionZ = random16();

    ctx.state.speedX = random8();
    ctx.state.speedY = random8();
    ctx.state.speedZ = random8();
    ctx.state.scale = 600;

    ctx.state.ihue = 0;

    ctx.template set_config_bool<ConfigKeys::rampSaturates>(false);
    // this ramps should be slow
    ctx.template set_config_u32<ConfigKeys::customRampStepSpeedMs>(ctx.state.maxPalettesCount / 255.0f * 850);

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
    auto& state = ctx.state;
    auto& lamp = ctx.lamp;
    if (!state.selectedPalette)
      return;

    // reference to a value buffer
    static auto noiseBuffer = lamp.template getTempBuffer<bufferIndexToUse>();

    // copy to prevent a ramp update mid animation
    const palette_t palette = *(state.selectedPalette);

    const auto x = state.positionX;
    const auto y = state.positionY;
    const auto z = state.positionZ;
    const auto scale = state.scale;

    // this is too heavy to run at full, speed, display every other pixels instead of refreshing amm
    static constexpr size_t everyNIndex = 2;
    const size_t firstIndex = lamp.tick % everyNIndex;

    // update noise values
    for (size_t i = firstIndex; i < lamp.ledCount; i += everyNIndex)
    {
      // TODO #150: use new strip interface coordinates
      const auto res = lamp.getLegacyStrip().get_lamp_coordinates(i);
      uint16_t data = noise16::inoise(x + scale * res.x, y + scale * res.y, z + scale * res.z);

      // smooth over time to prevent suddent jumps
      noiseBuffer[i] = data;
    }

    // apply slow drift to X and Y, just for visual variation.
    state.positionX += state.speedX;
    state.positionY += state.speedY;
    state.positionZ += state.speedZ;

    // increase speed by a random amount
    state.speedX = lmpd_constrain(
            state.speedX + lmpd_map<uint8_t, int16_t>(random8(), 0, 255, -25, 25), -state.maxSpeed, state.maxSpeed);
    state.speedY = lmpd_constrain(
            state.speedY + lmpd_map<uint8_t, int16_t>(random8(), 0, 255, -25, 25), -state.maxSpeed, state.maxSpeed);
    state.speedZ = lmpd_constrain(
            state.speedZ + lmpd_map<uint8_t, int16_t>(random8(), 0, 255, -25, 25), -state.maxSpeed, state.maxSpeed);
    // prevent too slow speed
    if (state.speedX < 0 and state.speedX > -state.minSpeed)
      state.speedX = -state.minSpeed;
    if (state.speedY < 0 and state.speedY > -state.minSpeed)
      state.speedY = -state.minSpeed;
    if (state.speedZ < 0 and state.speedZ > -state.minSpeed)
      state.speedZ = -state.minSpeed;
    if (state.speedX > 0 and state.speedX < state.minSpeed)
      state.speedX = state.minSpeed;
    if (state.speedY > 0 and state.speedY < state.minSpeed)
      state.speedY = state.minSpeed;
    if (state.speedZ > 0 and state.speedZ < state.minSpeed)
      state.speedZ = state.minSpeed;

    for (size_t i = firstIndex; i < lamp.ledCount; i += everyNIndex)
    {
      uint16_t index = noiseBuffer[i];
      uint8_t bri = noiseBuffer[lamp.ledCount - 1 - i] >> 8;

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

      lamp.setPixelColor(i, colors::from_palette(index, palette, bri));
    }

    state.ihue += 1;
  }
};

} // namespace modes::default_modes

#endif

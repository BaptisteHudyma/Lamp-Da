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
namespace lampda::modes::default_modes {

/**
 * \brief 3D perlin noise on the lamp surface.
 */
struct PerlinNoiseMode : public BasicMode
{
  /// hint manager to save our custom ramp
  static constexpr bool hasCustomRamp = true;

  /// index of the buffer used in this mode
  static constexpr uint8_t bufferIndexToUse = 0;

  // this is too heavy to run at full, speed, display every other pixels instead of refreshing amm
  static constexpr uint32_t everyNIndex = 2;

  struct StateTy
  {
    uint32_t positionX; ///< position of the noise in X
    uint32_t positionY; ///< position of the noise in X
    uint32_t positionZ; ///< position of the noise in X

    int16_t speedX; ///< Speed of the noise in X
    int16_t speedY; ///< Speed of the noise in Y
    int16_t speedZ; ///< Speed of the noise in Z
    /// Minimal speed of movement
    static constexpr uint16_t minSpeed = 25;
    /// Maximal speed of movement
    static constexpr uint16_t maxSpeed = 400;
    /// Scale of the noise
    uint16_t scale;
    /// Color grading
    uint16_t ihue;

    /// count of palette used
    static constexpr uint8_t maxPalettesCount = 4;
    /// store references to palettes
    const colors::PaletteTy* _palettes[maxPalettesCount] = {&colors::PaletteRainbowColors,
                                                            &colors::PaletteLavaColors,
                                                            &colors::PaletteForestColors,
                                                            &colors::PaletteOceanColors};

    /// store selected palette
    colors::PaletteTy const* selectedPalette;

    // flag that we have just been reseted
    bool isResetted = false;
  };

  static void on_enter_mode(auto& ctx)
  {
    ctx.state.isResetted = true;

    ctx.state.positionX = UINT32_MAX / 2 + random16() / 2;
    ctx.state.positionY = UINT32_MAX / 2 + random16() / 2;
    ctx.state.positionZ = UINT32_MAX / 2 + random16() / 2;

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

  /// User ramp changes the color palette
  static void custom_ramp_update(auto& ctx, uint8_t rampValue)
  {
    auto& state = ctx.state;

    const uint8_t rampIndex =
            std::min<float>(floorf(rampValue / 255.0f * state.maxPalettesCount), state.maxPalettesCount - 1.0f);
    state.selectedPalette = state._palettes[rampIndex];
  }

  /**
   * \brief compute the next speed from the current parameters. This function prevent overflow by controling the speed
   * between two bounds.
   * \param[in] ctx Contrext object of the mode
   * \param[in] position Actual position of the noise
   * \param[in] speed Actual speed of the noise
   * \return The nex speed
   */
  static int16_t get_next_speed(auto& ctx, const uint32_t position, int16_t speed)
  {
    static constexpr float speedDivider = ctx.lamp.frameDurationMs / (1000.0 / 40.0);
    static constexpr int16_t speedBleedof = 25 * speedDivider;
    static constexpr int16_t acceleration = 25 * speedDivider;
    static constexpr uint16_t maxSpeed = ctx.state.maxSpeed * speedDivider;
    static constexpr uint16_t minSpeed = ctx.state.minSpeed * speedDivider;

    static constexpr uint32_t confortZone = UINT32_MAX / 200000;

    // close to zero, set mostly positive speeds
    if (position < confortZone)
    {
      // bleed off speed to zero
      if (speed < 0)
        speed += speedBleedof;
      else
        speed = lmpd_constrain<int16_t>(speed + lmpd_map<int16_t>(random8(), 0, 255, 0, acceleration * 2), 0, maxSpeed);
    }
    else if (position > (UINT32_MAX - confortZone))
    {
      // bleed off speed to zero
      if (speed > 0)
        speed -= speedBleedof;
      else
        speed = lmpd_constrain<int16_t>(
                speed + lmpd_map<int16_t>(random8(), 0, 255, -acceleration * 2, 0), -maxSpeed, 0);
    }
    else
    {
      // free range x speed
      speed = lmpd_constrain<int16_t>(
              speed + lmpd_map<int16_t>(random8(), 0, 255, -acceleration, acceleration), -maxSpeed, maxSpeed);
      // prevent too slow speed
      if (speed < 0 and speed > -minSpeed)
        speed = -minSpeed;
      else if (speed > 0 and speed < minSpeed)
        speed = minSpeed;
    }

    return speed;
  }

  static void loop(auto& ctx)
  {
    if (ctx.state.isResetted)
    {
      ctx.state.isResetted = false;

      // load animation
      for (uint32_t i = 0; i <= everyNIndex; ++i)
      {
        perlin_display(ctx, i);
      }
      return;
    }

    perlin_display(ctx, ctx.lamp.tick);
  }

  static void perlin_display(auto& ctx, const uint32_t tick)
  {
    auto& state = ctx.state;
    auto& lamp = ctx.lamp;
    if (!state.selectedPalette)
      return;

    // reference to a value buffer
    static auto noiseBuffer = lamp.template getTempBuffer<bufferIndexToUse>();

    // copy to prevent a ramp update mid animation
    const colors::PaletteTy palette = *(state.selectedPalette);

    const auto x = state.positionX;
    const auto y = state.positionY;
    const auto z = state.positionZ;
    const auto scale = state.scale;

    const size_t firstIndex = tick % everyNIndex;

    // update noise values
    for (size_t i = firstIndex; i < lamp.ledCount; i += everyNIndex)
    {
      const auto res = modes::strip_to_helix_unconstraint(i);
      uint16_t data = noise16::inoise(x + scale * res.x, y + scale * res.y, z + scale * res.z);

      // smooth over time to prevent suddent jumps
      noiseBuffer[i] = data;
    }

    // apply slow drift to X and Y, just for visual variation.
    state.positionX += state.speedX;
    state.positionY += state.speedY;
    state.positionZ += state.speedZ;

    // vary speed
    state.speedX = get_next_speed(ctx, state.positionX, state.speedX);
    state.speedY = get_next_speed(ctx, state.positionY, state.speedY);
    state.speedZ = get_next_speed(ctx, state.positionZ, state.speedZ);

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

} // namespace lampda::modes::default_modes

#endif

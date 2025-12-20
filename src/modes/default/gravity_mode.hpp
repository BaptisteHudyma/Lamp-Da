#ifndef GRAVITY_MODE_H
#define GRAVITY_MODE_H

/// @file gravity_mode.hpp

#include "src/modes/include/colors/palettes.hpp"
#include <cstddef>
#include <cstdint>
#include <functional>

namespace modes::default_modes {

/// Emulate a beat sync
struct GravityMode : public BasicMode
{
  // hint manager to save our custom ramp
  static constexpr bool hasCustomRamp = true;

  static constexpr uint16_t particleCount = 255;

  static void on_enter_mode(auto& ctx)
  {
    ctx.imuEvent.reset(ctx);
    // set particle count
    ctx.imuEvent.particuleSystem.set_max_particle_count(particleCount);
    ctx.imuEvent.particuleSystem.init_particules(generate_random_particule_position);

    ctx.state.persistance = 210;

    // set default value
    custom_ramp_update(ctx, ctx.get_active_custom_ramp());
  }

  static void custom_ramp_update(auto& ctx, uint8_t rampValue)
  {
    auto& state = ctx.state;

    const uint8_t rampIndex =
            min<uint8_t>(floorf(rampValue / 255.0f * state.maxPalettesCount), state.maxPalettesCount - 1.0f);

    state.selectedPalette = state._palettes[rampIndex];
  }

  static void loop(auto& ctx)
  {
    auto& state = ctx.state;
    if (!state.selectedPalette)
      return;

    const uint8_t paletteWrap = ctx.lamp.tick % UINT8_MAX;

    ctx.imuEvent.update(ctx);

    auto& particleSystem = ctx.imuEvent.particuleSystem;

    particleSystem.iterate_with_collisions(ctx.imuEvent.lastReading.accel, ctx.lamp.frameDurationMs / 1000.0);

    ctx.lamp.fadeToBlackBy(255 - ctx.state.persistance);

    auto colorFunction = [&](int16_t n, const Particle& particle) {
      const auto wrapIndex = (n + paletteWrap) % particleCount;
      return colors::from_palette(static_cast<uint8_t>(wrapIndex / static_cast<float>(particleCount) * UINT8_MAX),
                                  *state.selectedPalette);
    };
    particleSystem.show(colorFunction, ctx.lamp);
  }

  struct StateTy
  {
    uint8_t persistance;

    // store references to palettes
    static constexpr uint8_t maxPalettesCount = 3;
    const palette_t* _palettes[maxPalettesCount] = {
            &colors::PaletteAuroraColors, &colors::PaletteForestColors, &colors::PaletteOceanColors};

    // store selected palette
    palette_t const* selectedPalette;
  };

private:
  using LampTy = hardware::LampTy;

  /// spawn random particles
  static int16_t generate_random_particule_position(size_t) { return random16(LampTy::ledCount); }
  /// spawn particles at the bottom
  static int16_t generate_particule_at_top_random_position(size_t)
  {
    return -static_cast<float>(random16(2.0 * LampTy::maxWidth));
  }
  /// spawn particles at the bottom
  static int16_t generate_particule_at_bottom_random_position(size_t)
  {
    return LampTy::ledCount + static_cast<float>(random16(2.0 * LampTy::maxWidth));
  }

  /// pair particules spwan at the top, odd at bottom
  static int16_t generate_particule_at_extremes(size_t i)
  {
    return (i % 2 == 0) ? generate_particule_at_top_random_position(i) :
                          generate_particule_at_bottom_random_position(i);
  }

  /// Return true if the given particle needs to be recycled (out of bounds)
  static bool recycle_particules_if_too_far(const Particle& p)
  {
    return p.z_mm > LampTy::maxWidth * 3 or p.z_mm < -(LampTy::lampHeight_mm + LampTy::maxWidth * 3);
  }
};

} // namespace modes::default_modes

#endif // BEATSYNC_MODE_H

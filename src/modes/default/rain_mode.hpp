#ifndef RAIN_MODE_H
#define RAIN_MODE_H

/// @file vumeter
#include "src/modes/include/colors/palettes.hpp"
#include <cstddef>
#include <cstdint>

namespace modes::default_modes {

/// Emulate a beat sync
struct RainMode : public BasicMode
{
  // hint manager to save our custom ramp
  static constexpr bool hasCustomRamp = true;

  static constexpr float lightRainDropsPerSecond = 1;
  static constexpr float heavyRainDropsPerSecond = 700;

  static void on_enter_mode(auto& ctx)
  {
    ctx.imuEvent.reset(ctx);
    // set particle count
    ctx.imuEvent.particuleSystem.set_max_particle_count(512);

    // ramp saturates
    ctx.template set_config_bool<ConfigKeys::rampSaturates>(true);

    ctx.state.rainDropSpawn = 0.0;
    ctx.state.persistance = 100;
    ctx.state.rainDensity = 1;

    // set default value
    custom_ramp_update(ctx, ctx.get_active_custom_ramp());
  }

  static void custom_ramp_update(auto& ctx, uint8_t rampValue) { ctx.state.rainDensity = rampValue; }

  static void loop(auto& ctx)
  {
    auto& state = ctx.state;
    ctx.imuEvent.update(ctx);

    auto& particleSystem = ctx.imuEvent.particuleSystem;

    // multiply by 2 because we rain on both sides of the lamp, half of the drops are not seen
    const float expectedDropPerSecond =
            ctx.state.rainDensity / 255.0 * heavyRainDropsPerSecond + lightRainDropsPerSecond;
    ctx.state.rainDropSpawn += expectedDropPerSecond * ctx.lamp.frameDurationMs / 1000.0;

    if (ctx.state.rainDropSpawn > 1.0)
    {
      // initialize particules in a deffered way, when free spots are available
      particleSystem.init_deferred_particules(2 * ctx.state.rainDropSpawn, generate_particule_at_extremes);
      ctx.state.rainDropSpawn = 0.0;
    }

    // no collisions between particles, and with no lamp limits
    static constexpr bool shouldKeepInLampBounds = false;
    particleSystem.iterate_no_collisions(
            ctx.imuEvent.lastReading.accel, ctx.lamp.frameDurationMs / 1000.0, shouldKeepInLampBounds);
    // depop particules that fell too far
    particleSystem.depop_particules(recycle_particules_if_too_far);

    ctx.lamp.fadeToBlackBy(255 - ctx.state.persistance);

    // break palette size to display more colors per drops
    auto colorFunction = [&](int16_t n, const Particle& particle) {
      return colors::from_palette(static_cast<uint8_t>(n % 16), ctx.state.palette);
    };
    particleSystem.show(colorFunction, ctx.lamp);
  }

  struct StateTy
  {
    uint8_t rainDensity;
    uint8_t persistance;
    float rainDropSpawn;

    const palette_t& palette = colors::PaletteWaterColors;
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

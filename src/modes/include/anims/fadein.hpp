/*! \file fadein.hpp
    \brief Fade-in animation: pixels fall from the top row and unmask the pixel where they land.
*/

#ifndef MODES_ANIM_FADEIN_HPP
#define MODES_ANIM_FADEIN_HPP

#include <limits>
#include <type_traits>

#include "src/modes/include/particle_system/particle_system.hpp"

namespace lampda::modes::anims {
/// Define fadein animations. The whole animation starts hidden by the lamp mask.
namespace fadein {

/**
 * \brief Make an animation appear with gravity.
 * Particles spawn on a hidden pixel of the top line and fall. A particle lands on the first hidden pixel that has
 * no room left below it (last line, or a visible pixel). It then unmasks this pixel and despawns.
 * The state is only the mask and two counters, so there is no dynamic memory.
 */
struct GravityFill
{
  void reset(auto& ctx)
  {
    // hide the whole animation
    ctx.lamp.mask_all_leds(true);

    particuleSystem.reset();
    particuleSystem.set_max_particle_count(20);
    revealedCount = 0;
    targetCount = 0.0f;
  }

  /// \param[in] progress Current progress, from 0.0 to 1.0
  void update_progress_rate(auto& ctx, const float progress)
  {
    targetCount = static_cast<float>(ctx.lamp.ledCount) * lmpd_constrain<float>(progress / gracePeriod, 0.0f, 1.0f);
  }

  void loop(auto& ctx)
  {
    // spawn what is missing: revealed pixels + pixels in flight must reach the target
    while (static_cast<float>(revealedCount + particuleSystem.get_number_of_active()) < targetCount and spawn_one(ctx))
    {
    }

    if (particuleSystem.get_number_of_active() == 0)
      return;

    static constexpr bool shouldKeepInLampBounds = false;
    particuleSystem.iterate_no_collisions(
            utils::vec3d(0.0, 0.0, -9.81 / 4.0), ctx.lamp.frameDurationMs / 1000.0, shouldKeepInLampBounds);

    // unmask and despawn the particles that landed
    particuleSystem.depop_particules([this, &ctx](const Particle& p) {
      return land(ctx, p);
    });

    // the mask hides the pixels: force display, and sample the animation color under each particle
    static constexpr bool forceDisplay = true;
    particuleSystem.show(
            [&ctx](int16_t, const Particle& p) -> uint32_t {
              /// TODO: spawn other than white
              return 0xFFFFFF;
            },
            ctx.lamp,
            forceDisplay);
  }

private:
  static constexpr float gracePeriod = 0.9f; ///< Spawns end before the progress does, to let the last ones land
  static constexpr float rowPitch_mm = LampTy::ledStripWidth_mm * 1.5f; ///< Height of one pixel

  /// Index of the pixel `dRows` lines below (negative: above) the particle, following its angle
  static int16_t pixel_at(const Particle& p, const float dRows)
  {
    return Particle(p.theta_rad, p.z_mm - dRows * rowPitch_mm).to_lamp_index_no_bounds();
  }

  static bool is_visible(auto& ctx, const int16_t id) { return is_led_index_valid(id) and ctx.lamp.ledMask[id]; }
  static bool is_hidden(auto& ctx, const int16_t id) { return is_led_index_valid(id) and not ctx.lamp.ledMask[id]; }

  void reveal(auto& ctx, const int16_t id)
  {
    ctx.lamp.ledMask.set(id);
    revealedCount += 1;
  }

  /// Depop predicate: return true once the particle landed (or is lost) and unmask its target
  bool land(auto& ctx, const Particle& p)
  {
    const int16_t here = p._savedLampIndex;
    if (is_hidden(ctx, here))
    {
      const int16_t below = pixel_at(p, 1.0f);
      if (is_visible(ctx, below) or not is_led_index_valid(below))
      {
        reveal(ctx, here); // no room below: this is the target
        return true;
      }
      return false; // room below: keep falling
    }

    // Skipped the target (fast fall, or two particles in one column): fill the first hidden pixel above.
    // If none is found, the particle is dropped, and the spawn logic compensates.
    for (int16_t row = 1; row <= LampTy::maxHeight + 1; row++)
    {
      const int16_t above = pixel_at(p, -static_cast<float>(row));
      if (is_hidden(ctx, above))
      {
        reveal(ctx, above);
        break;
      }
    }
    return true;
  }

  /// Id of the top pixel of column x if it is still hidden, -1 otherwise
  static int16_t hidden_top_pixel(auto& ctx, const uint16_t x)
  {
    const auto id = modes::to_strip(x, 0);
    return (id >= 0 and id < ctx.lamp.ledCount and not ctx.lamp.ledMask[id]) ? static_cast<int16_t>(id) : -1;
  }

  /// Spawn one particle on a random hidden pixel of the top line
  bool spawn_one(auto& ctx)
  {
    uint16_t candidates = 0;
    for (uint16_t x = 0; x <= ctx.lamp.maxWidth; x++)
      candidates += hidden_top_pixel(ctx, x) >= 0 ? 1 : 0;
    if (candidates == 0)
      return false;

    uint16_t selected = lmpd_map<uint16_t>(rand(), 0, RAND_MAX, 0, candidates);
    for (uint16_t x = 0; x <= ctx.lamp.maxWidth; x++)
    {
      const int16_t pixelId = hidden_top_pixel(ctx, x);
      if (pixelId < 0)
        continue;
      if (selected-- > 0)
        continue;

      // returns true when the particle could NOT be created (pool full): retry on next loop
      const bool isFailed = particuleSystem.init_deferred_particules(1, [pixelId](size_t) {
        return pixelId;
      });
      return not isFailed;
    }
    return false;
  }

  size_t revealedCount = 0; ///< pixels unmasked so far
  float targetCount = 0.0f; ///< pixels that should be revealed or in flight at the current progress

  modes::ParticleSystem<20> particuleSystem;
};

} // namespace fadein
} // namespace lampda::modes::anims

#endif

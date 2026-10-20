/*! \file fadein.hpp
    \brief Fade-in animation: pixels fall from the top and unmask the pixel where they land.
*/

#ifndef MODES_ANIM_FADEIN_HPP
#define MODES_ANIM_FADEIN_HPP

#include <array>
#include <cstdint>
#include <limits>
#include <type_traits>

#include "src/modes/include/particle_system/particle_system.hpp"

namespace lampda::modes::anims {
/// Define fadein animations. The whole animation starts hidden by the lamp mask.
namespace fadein {

/**
 * \brief Make an animation appear with gravity.
 * Particles spawn on a hidden pixel of the highest line that still has one, and fall.
 * A particle unmasks the last hidden pixel it reaches before a visible pixel (or the lamp end).
 * The spawn pixel is chosen where the free space below it is the largest, so the fill stays homogeneous.
 * No dynamic memory: only the mask, two counters and a small ring buffer are stored.
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
    recentSpawns.fill(noSpawn);
    recentSpawnHead = 0;
  }

  /// \param[in] progress Current progress, from 0.0 to 1.0
  void update_progress_rate(auto& ctx, const float progress)
  {
    targetCount = static_cast<float>(ctx.lamp.ledCount) * lmpd_constrain<float>(progress / gracePeriod, 0.0f, 1.0f);
  }

  void loop(auto& ctx)
  {
    // revealed pixels + pixels in flight must reach the target
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
              return 0xFFFFFF;
            },
            ctx.lamp,
            forceDisplay);
  }

private:
  static constexpr float gracePeriod = 0.9f; ///< Spawns end before the progress does, to let the last ones land

  static constexpr float pixelUnit_mm = LampTy::ledStripWidth_mm * 1.5f; ///< Height of one pixel
  static constexpr float stepMm = pixelUnit_mm / 4.0f;                   ///< Sampling step, finer than a pixel
  static constexpr uint8_t maxNeighbourSteps = 16;                       ///< Search range for the next pixel
  static constexpr uint16_t maxUpSteps = static_cast<uint16_t>(LampTy::lampHeight_mm / stepMm) + 8;

  static constexpr uint8_t recentSpawnCount = 4;  ///< Spawns remembered, to spread the particles in flight
  static constexpr int16_t noSpawn = -100;        ///< Empty slot in the ring buffer
  static constexpr int32_t recentPenalty = 4 * 8; ///< Score penalty (in steps) per recent spawn on a column

  static bool is_hidden(auto& ctx, const int32_t id)
  {
    return id >= 0 and id < ctx.lamp.ledCount and not ctx.lamp.ledMask[id];
  }

  /// Pixel reached when moving from the particle along z (dir = -1: down, +1: up). -1 if none found.
  static int16_t next_pixel(const Particle& p, const int16_t from, const float dir)
  {
    for (uint8_t i = 1; i <= maxNeighbourSteps; i++)
    {
      const int16_t id = to_led_index_no_bounds(p.theta_rad, p.z_mm + dir * i * stepMm);
      if (id != from)
        return id;
    }
    return -1;
  }

  /// First hidden pixel above the particle, along its angle. -1 if none found.
  static int16_t first_hidden_above(auto& ctx, const Particle& p)
  {
    for (uint16_t i = 1; i <= maxUpSteps; i++)
    {
      const int16_t id = to_led_index_no_bounds(p.theta_rad, p.z_mm + i * stepMm);
      if (is_hidden(ctx, id))
        return id;
    }
    return -1;
  }

  /// Free space below a pixel, in sampling steps: how far a particle falls before meeting a visible pixel.
  /// The larger it is, the emptier the area.
  static int32_t free_depth(auto& ctx, const int16_t pixelId)
  {
    const auto helix = modes::strip_to_helix_unconstraint(pixelId);
    const Particle p(utils::vec3d {helix.x, helix.y, helix.z});

    int32_t steps = 0;
    while (steps < maxUpSteps and is_hidden(ctx, to_led_index_no_bounds(p.theta_rad, p.z_mm - steps * stepMm)))
      steps += 1;
    return steps;
  }

  /// Penalty for the particles recently spawned on this column or its direct neighbours
  int32_t spawn_penalty(auto& ctx, const int16_t x) const
  {
    const int16_t width = ctx.lamp.maxWidth + 1;
    int32_t penalty = 0;
    for (const int16_t recentX: recentSpawns)
    {
      if (recentX == noSpawn)
        continue;
      int16_t distance = recentX > x ? recentX - x : x - recentX;
      distance = distance < width - distance ? distance : width - distance; // the lamp is circular
      if (distance <= 1)
        penalty += recentPenalty;
    }
    return penalty;
  }

  void reveal(auto& ctx, const int16_t id)
  {
    ctx.lamp.ledMask.set(id);
    revealedCount += 1;
  }

  /// Depop predicate: return true when the particle is done, after unmasking its target
  bool land(auto& ctx, const Particle& p)
  {
    const int16_t here = p._savedLampIndex;

    if (is_hidden(ctx, here))
    {
      // room below: keep falling
      if (is_hidden(ctx, next_pixel(p, here, -1.0f)))
        return false;
      // visible pixel or lamp end below: this is the target
      reveal(ctx, here);
      return true;
    }

    // Went through its target in one step (or left the lamp): fill the first hidden pixel above.
    // If none exists, the particle is dropped and the spawn logic compensates.
    const int16_t above = first_hidden_above(ctx, p);
    if (above >= 0)
      reveal(ctx, above);
    return true;
  }

  /// Spawn one particle where the free space below is the largest (random among equals)
  bool spawn_one(auto& ctx)
  {
    for (int16_t y = 0; y <= ctx.lamp.maxHeight; y++)
    {
      int16_t bestId = -1;
      int16_t bestX = 0;
      int32_t bestScore = std::numeric_limits<int32_t>::min();
      uint16_t ties = 0;

      for (uint16_t x = 0; x <= ctx.lamp.maxWidth; x++)
      {
        const int16_t id = modes::to_strip(x, y);
        if (not is_hidden(ctx, id))
          continue;

        const int32_t score = free_depth(ctx, id); // - spawn_penalty(ctx, x);
        if (score > bestScore)
        {
          bestScore = score;
          bestId = id;
          bestX = x;
          ties = 1;
        }
        else if (score == bestScore)
        {
          // reservoir sampling: uniform choice among equal scores, without storing them
          ties += 1;
          if (static_cast<uint16_t>(rand()) % ties == 0)
          {
            bestId = id;
            bestX = x;
          }
        }
      }

      if (bestId < 0)
        continue; // nothing hidden on this line, try the next one

      // returns true when the particle could NOT be created (pool full): retry on next loop
      const bool isFailed = particuleSystem.init_deferred_particules(1, [bestId](size_t) {
        return bestId;
      });
      if (isFailed)
        return false;

      recentSpawns[recentSpawnHead] = bestX;
      recentSpawnHead = (recentSpawnHead + 1) % recentSpawnCount;
      return true;
    }
    return false;
  }

  size_t revealedCount = 0; ///< pixels unmasked so far
  float targetCount = 0.0f; ///< pixels that should be revealed or in flight at the current progress

  std::array<int16_t, recentSpawnCount> recentSpawns {}; ///< columns of the latest spawns (ring buffer)
  uint8_t recentSpawnHead = 0;

  modes::ParticleSystem<20> particuleSystem;
};

} // namespace fadein
} // namespace lampda::modes::anims

#endif

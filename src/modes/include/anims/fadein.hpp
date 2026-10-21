/*! \file fadein.hpp
    \brief Define some animations to fade in modes.
*/

#ifndef MODES_ANIM_FADEIN_HPP
#define MODES_ANIM_FADEIN_HPP

#include <array>
#include <cstdlib>
#include <etl/bitset.h>

#include "src/modes/include/particle_system/particle_system.hpp"

namespace lampda::modes::anims {
namespace fadein {

/**
 * \brief Reveal an animation with falling pixels.
 *
 * The whole lamp starts masked. Particles spawn at the top, fall, and unmask the pixel they land on.
 * Landing targets are picked line by line from the bottom to the top (random inside the line), so the
 * reveal rises uniformly: no empty column, no clump higher than one line.
 * The spawn rate follows the (extrapolated) progress, and the lamp is fully unmasked when progress reaches 1.
 * \param MaxParticles Maximum number of particles falling at the same time (throughput limit)
 */
template<size_t MaxParticles = 20> struct GravityFadeIn
{
  void reset(auto& ctx)
  {
    maskedLeds.set(); // everything hidden

    particuleSystem.reset();
    particuleSystem.set_max_particle_count(MaxParticles);
    reserved.reset();
    targets.fill(0);
    targetsZ.fill(0.0f);

    latestProgress = -1.0f;
    latestProgressTime = 0;
    progressPerMs = 0.0f;
    spawned = 0;
    isComplete = false;
  }

  void loop(auto& ctx)
  {
    if (isComplete)
      return;

    spawn_pending(ctx);

    // mask all the leds that needs masking
    for (uint16_t i = 0; i < maskedLeds.size(); i++)
    {
      if (maskedLeds[i])
        ctx.lamp.mask_led_at_index(i, true);
    }

    if (particuleSystem.get_number_of_active() > 0)
    {
      static constexpr bool shouldKeepInLampBounds = false;
      particuleSystem.iterate_no_collisions(
              utils::vec3d(0.0, 0.0, -9.81 / 4.0), ctx.lamp.frameDurationMs / 1000.0, shouldKeepInLampBounds);

      // unmask and remove the particles that reached their target
      const uint16_t left = particuleSystem.depop_particules_indexed([&](size_t slot, const Particle& p) {
        return land_if_arrived(ctx, slot, p);
      });

      // Force display because the pixels below are masked
      static constexpr bool forceDisplayParticles = true;
      particuleSystem.show(ctx.lamp, forceDisplayParticles);

      if (left == 0 and spawned >= ctx.lamp.ledCount)
        isComplete = true;
    }
  }

  /**
   * \brief Update the drop rate
   * \param[in] expectedProgress Current progress, from 0 to 1 (may start at any value)
   */
  void update_progress_rate(auto& ctx, const float expectedProgress)
  {
    if (isComplete)
      return;

    const float progress = lmpd_constrain<float>(expectedProgress, 0.0f, 1.0f);

    // done: reveal everything, whatever is still falling
    if (progress >= 1.0f)
    {
      complete(ctx);
      return;
    }

    if (progress == latestProgress)
      return;

    if (latestProgress >= 0.0f and ctx.lamp.now > latestProgressTime)
    {
      const float elapsedMs = static_cast<float>(ctx.lamp.now - latestProgressTime);
      progressPerMs = lmpd_constrain<float>((progress - latestProgress) / elapsedMs, 0.0f, 1.0f);
    }
    else
    {
      progressPerMs = 0.0f; // first call: rate unknown, only catch up to the current progress
    }
    latestProgress = progress;
    latestProgressTime = ctx.lamp.now;
  }

protected:
  /// Progress estimated "now", including the fall duration so that pixels land on time
  float estimated_progress(auto& ctx) const
  {
    if (latestProgress < 0.0f)
      return 0.0f;
    const float elapsedMs = static_cast<float>(ctx.lamp.now - latestProgressTime) + fallLeadMs;
    return lmpd_constrain<float>(latestProgress + progressPerMs * elapsedMs, latestProgress, 1.0f);
  }

  void complete(auto& ctx)
  {
    maskedLeds.reset(); // everything visible

    particuleSystem.reset();
    reserved.reset();
    spawned = ctx.lamp.ledCount;
    isComplete = true;
  }

  /// Spawn the particles needed to follow the estimated progress
  void spawn_pending(auto& ctx)
  {
    const uint16_t total = ctx.lamp.ledCount;
    const uint16_t wanted = static_cast<uint16_t>(estimated_progress(ctx) * total + 0.5f);

    // limit the spawns per frame: no burst, particles are spread over time
    for (uint8_t i = 0; i < maxSpawnPerLoop and spawned < wanted; i++)
    {
      if (not spawn_one(ctx))
        break; // no free slot or no free target: retry next frame
    }
  }

  bool spawn_one(auto& ctx)
  {
    if (particuleSystem.get_number_of_active() >= MaxParticles)
      return false;

    uint32_t targetId;
    if (not pick_target(ctx, targetId))
      return false;

    // spawn at the top of the lamp, same angle as the target
    const auto helix = modes::strip_to_helix_unconstraint(static_cast<int16_t>(targetId));
    const float theta = atan2(helix.y, helix.x);
    const int16_t spawnIndex = modes::to_led_index_no_bounds(theta, 0.0f);
    const float targetZ = helix.z;
    const uint32_t color = ctx.lamp.getPixelColor(targetId);

    const bool notAllCreated = particuleSystem.init_deferred_particules(
            1,
            [this, targetId, targetZ, spawnIndex](size_t slot) -> int16_t {
              this->targets[slot] = targetId;
              this->targetsZ[slot] = targetZ;
              return spawnIndex;
            },
            [color](size_t) {
              return color;
            });
    if (notAllCreated)
      return false; // no free slot

    reserved.set(targetId);
    spawned += 1;
    return true;
  }

  /// A masked pixel not already targeted by another particle
  bool is_candidate(auto& ctx, uint16_t x, int16_t y)
  {
    const auto id = modes::to_strip(x, y);
    if (id < 0 or id >= ctx.lamp.ledCount)
      return false;
    return maskedLeds[id] and not reserved[id];
  }

  /// Pick a random target in the lowest line that still has free pixels
  bool pick_target(auto& ctx, uint32_t& targetId)
  {
    for (int16_t y = ctx.lamp.maxHeight; y >= 0; y--)
    {
      uint16_t count = 0;
      for (uint16_t x = 0; x <= ctx.lamp.maxWidth; x++)
      {
        if (is_candidate(ctx, x, y))
          count += 1;
      }
      if (count == 0)
        continue;

      uint16_t selected = static_cast<uint16_t>(rand() % count);
      for (uint16_t x = 0; x <= ctx.lamp.maxWidth; x++)
      {
        if (not is_candidate(ctx, x, y))
          continue;
        if (selected == 0)
        {
          targetId = static_cast<uint32_t>(modes::to_strip(x, y));
          return true;
        }
        selected -= 1;
      }
    }
    return false; // every pixel is revealed or targeted
  }

  /// Unmask the target and remove the particle once it has reached the target height
  bool land_if_arrived(auto& ctx, const size_t slot, const Particle& p)
  {
    // z goes from 0 (top) to -lampHeight (bottom)
    if (p.z_mm > targetsZ[slot])
      return false;

    const uint32_t target = targets[slot];
    maskedLeds.reset(target); // visible
    reserved.reset(target);
    return true;
  }

private:
  static constexpr float fallLeadMs = 300.0f;   ///< Estimated fall duration, tune with the lamp height
  static constexpr uint8_t maxSpawnPerLoop = 2; ///< Max particles spawned per loop call

  float latestProgress = -1.0f;
  uint32_t latestProgressTime = 0;
  float progressPerMs = 0.0f;
  size_t spawned = 0; ///< particles already spawned (fallen or falling)
  bool isComplete = false;

  std::array<uint32_t, MaxParticles> targets {}; ///< target pixel of each particle slot
  std::array<float, MaxParticles> targetsZ {};   ///< target height of each particle slot
  etl::bitset<LampTy::ledCount> reserved;        ///< pixels already targeted by a falling particle
  etl::bitset<LampTy::ledCount> maskedLeds;      /// keep track of the already masked leds

  modes::ParticleSystem<MaxParticles> particuleSystem;
};

} // namespace fadein
} // namespace lampda::modes::anims

#endif

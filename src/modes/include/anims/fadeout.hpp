/*! \file fadeout.hpp
    \brief Define some animations to fade out modes.
*/

#ifndef MODES_ANIM_FADEOUT_HPP
#define MODES_ANIM_FADEOUT_HPP

#include "src/modes/include/particle_system/particle_system.hpp"

namespace lampda::modes::anims {
/// Define fadout animation. They require a buffer ID, used to store the fadout mask
namespace fadeout {

/**
 * Drop policies: choose the drop mode
 */

/**
 * \brief Drop mode policy: remove particles line by line from bottom to top
 * Only pixels from the lowest non-empty line can be dropped.
 */
struct DropModeLineByLine
{
  /**
   * \brief Find and depop one particle according to this mode
   * \return true if a particle was successfully depopped, false otherwise
   */
  template<typename ContextType, uint8_t MaskBuffId, uint8_t minMaskValue>
  static bool depop_one(ContextType& ctx, uint32_t& pixelId)
  {
    auto& buffer = ctx.lamp.template getTempBuffer<MaskBuffId>();
    const auto bufferSize = ctx.lamp.ledCount;

    // check lines by lines from bottom to top, keeping the first one with pixels left
    for (int16_t y = ctx.lamp.maxHeight; y >= 0; y--)
    {
      // count pixels set in this line
      uint8_t pixelSetCount = 0;
      for (uint16_t x = 0; x <= ctx.lamp.maxWidth; x++)
      {
        const auto buffIndex = modes::to_strip(x, y);
        if (buffIndex < 0 or buffIndex >= bufferSize)
          continue;
        if (buffer[buffIndex] > minMaskValue)
          pixelSetCount += 1;
      }

      // found the first line with pixels left
      if (pixelSetCount > 0)
      {
        // select random pixel in this line
        uint8_t selectedLed = lmpd_map<uint8_t>(rand(), 0, RAND_MAX, 0, pixelSetCount);
        for (uint16_t x = 0; x <= ctx.lamp.maxWidth; x++)
        {
          const auto tempPixelId = modes::to_strip(x, y);
          if (tempPixelId < 0 or tempPixelId >= bufferSize)
            continue;
          if (buffer[tempPixelId] > minMaskValue)
          {
            if (selectedLed == 0)
            {
              pixelId = tempPixelId; // Return the pixel ID to depop
              return true;
            }
            selectedLed -= 1;
          }
        }
        break;
      }
    }
    return false; // No pixel found
  }
};

/**
 * \brief Drop mode policy: remove only "orphan" particles
 * A particle can only be dropped if there is no other particle directly below it (z-axis).
 */
struct DropModeOrphansOnly
{
  /**
   * \brief Check if a pixel at (x, y) has a pixel below it
   */
  template<typename ContextType, uint8_t MaskBuffId, uint8_t minMaskValue>
  static bool has_pixel_below(ContextType& ctx, uint16_t x, int16_t y)
  {
    auto& buffer = ctx.lamp.template getTempBuffer<MaskBuffId>();
    const auto bufferSize = ctx.lamp.ledCount;

    if (y >= ctx.lamp.maxHeight or (y + 1) > ctx.lamp.maxHeight)
    {
      return false;
    }

    // check the line below
    const auto pixelId = modes::to_strip(x, y + 1);
    if (pixelId >= 0 and pixelId < bufferSize)
    {
      if (buffer[pixelId] > minMaskValue)
        return true; // Found a pixel below
    }
    // check offsetted line, lamp is not wrapped straight !
    const auto pixelIdOffseted = modes::to_strip(x - 1, y + 1);
    if (pixelIdOffseted >= 0 and pixelIdOffseted < bufferSize)
    {
      if (buffer[pixelIdOffseted] > minMaskValue)
        return true; // Found a pixel below
    }
    return false;
  }

  template<typename ContextType, uint8_t MaskBuffId, uint8_t minMaskValue>
  static bool depop_one(ContextType& ctx, uint32_t& pixelId)
  {
    auto& buffer = ctx.lamp.template getTempBuffer<MaskBuffId>();
    const auto bufferSize = ctx.lamp.ledCount;
    uint8_t orphanCount = 0;

    int16_t minYLine = ctx.lamp.maxHeight;
    int16_t maxYLine = 0;

    // Count all orphan pixels
    for (int16_t y = ctx.lamp.maxHeight; y >= 0; y--)
    {
      bool hasSetPixels = false;
      bool hasOrphanPixels = false;
      for (uint16_t x = 0; x <= ctx.lamp.maxWidth; x++)
      {
        const auto tempPixelId = modes::to_strip(x, y);
        if (tempPixelId < 0 or tempPixelId >= bufferSize)
          continue;
        if (buffer[tempPixelId] > minMaskValue)
        {
          hasSetPixels = true;

          // Check if this pixel is an orphan (no pixel below it)
          if (not has_pixel_below<ContextType, MaskBuffId, minMaskValue>(ctx, x, y))
          {
            orphanCount += 1;
            hasOrphanPixels = true;

            minYLine = min<int16_t>(minYLine, y);
            maxYLine = max<int16_t>(maxYLine, y);
          }
        }
      }
      // line with pixels and no orphans, we can quit the check here
      if (hasSetPixels and not hasOrphanPixels)
      {
        break;
      }
    }

    if (orphanCount <= 0)
    {
      bsp::lampda_print("Could not find orphan nodes, skipping");
      return false;
    }

    uint8_t selectedLed = lmpd_map<uint8_t>(rand(), 0, RAND_MAX, 0, orphanCount);

    // drop a random orphan pixels
    for (int16_t y = maxYLine; y >= minYLine; y--)
    {
      for (uint16_t x = 0; x <= ctx.lamp.maxWidth; x++)
      {
        const auto tempPixelId = modes::to_strip(x, y);
        if (tempPixelId < 0 or tempPixelId >= bufferSize)
          continue;
        if (buffer[tempPixelId] > minMaskValue)
        {
          // Check if this pixel is an orphan (no pixel below it)
          if (not has_pixel_below<ContextType, MaskBuffId, minMaskValue>(ctx, x, y))
          {
            if (selectedLed == 0)
            {
              pixelId = tempPixelId; // Found the selected orphan
              return true;
            }
            selectedLed -= 1;
          }
        }
      }
    }
    return false; // No orphan pixel found
  }
};

/**
 * \brief Make an animation disapear with gravity
 * \param MaskBuffId Index of the buffer that will contain the mask values for the dropped leds
 * \param DropMode The drop policy, defined above (eg: DropModeLineByLine, DropModeOrphansOnly...)
 * \param minMaskValue Value to give to a pixel that gets droped
 */
template<uint8_t MaskBuffId, typename DropMode = DropModeOrphansOnly, uint8_t minMaskValue = 0> struct GravityDissolve
{
  void reset(auto& ctx)
  {
    // set all values to max: no masking
    ctx.lamp.template fillTempBuffer<MaskBuffId>(UINT8_MAX);
    // reset particles
    particuleSystem.reset();
    particuleSystem.set_max_particle_count(20);

    latestProgress = -1.0;
    latestProgressTime = 0;
    particlesToDepopPerIteration = 0.0;
    depopRate = 0.0;
    particlesDropped = 0;
  }

  void loop(auto& ctx, uint32_t color)
  {
    // find the particles to depop
    depopRate += particlesToDepopPerIteration;
    // time to depop one of more particle
    float integer;
    const float fractional = modf(depopRate, &integer);
    if (integer >= 1.0 and particlesDropped < ctx.lamp.ledCount)
    {
      // time to depop !
      for (uint8_t depopIndex = 0; depopIndex < integer; depopIndex++)
      {
        // depop one particle, yay !
        depop_one_particles(ctx);
      }

      // drop depop rate
      depopRate = fractional;
    }

    // no updates if no particles
    if (particuleSystem.get_number_of_active() > 0)
    {
      // depop particles out of bounds
      static constexpr bool shouldKeepInLampBounds = false;
      particuleSystem.iterate_no_collisions(
              utils::vec3d(0.0, 0.0, -9.81 / 4.0), ctx.lamp.frameDurationMs / 1000.0, shouldKeepInLampBounds);
      particuleSystem.depop_particules(recycle_particules_if_too_far);
      particuleSystem.show(
              [&](int16_t n, const Particle& particle) {
                return color;
              },
              ctx.lamp);
    }
  }

  /**
   * \brief Update the drop rate
   * \param[in] progress Current progress
   */
  void update_depop_rate(auto& ctx, const float expectedProgress)
  {
    // Artifically lenghten the progress to let particles disappear
    const float progress = expectedProgress / gracePeriod;

    const float updateDuration = static_cast<float>((ctx.lamp.now - latestProgressTime) / 1000.0);
    if (updateDuration <= 0.0f)
      return;

    const float progressPerSecond = (progress - latestProgress) / updateDuration;
    bool isValidCall = latestProgress >= 0 and updateDuration > 0;

    // cancel the particle spawn
    if (progress <= 0.0)
    {
      particlesToDepopPerIteration = 0.0;
      latestProgress = 0.0;
      isValidCall = false;
    }
    // update progress
    else if (latestProgress != progress)
    {
      latestProgress = progress;
      latestProgressTime = ctx.lamp.now;
    }
    else
      isValidCall = false;

    // not valid call, skip update
    if (not isValidCall)
    {
      return;
    }

    const uint16_t particleMax = ctx.lamp.ledCount;
    const float frameFrequency = 1000.0 / ctx.lamp.frameDurationMs;

    const float particlesLeft = particleMax - particlesDropped;
    const float trueProgress = 1.0 - particlesLeft / static_cast<float>(particleMax);

    // How many particles SHOULD have been dropped by now?
    const float expectedParticlesDropped = particleMax * progress;
    const float particleDeficit = expectedParticlesDropped - static_cast<float>(particlesDropped);

    const float remainingProgressRatio = 1.0 - progress;

    // SMOOTH CATCH-UP: Distribute deficit + normal depop over remaining progress window
    // This ensures we catch up gradually, not all at once
    if (remainingProgressRatio > 0.0f && particlesLeft > 0.0f)
    {
      // Normal depop rate (if we were on track)
      float normalDepopRate = progressPerSecond * particlesLeft / frameFrequency;

      // Spread the deficit evenly over the remaining progress window
      // deficit / (remaining_progress / progress_rate) = deficit / time_to_finish
      float deficitCatchupRate = 0.0f;
      if (progressPerSecond > 0.0f)
      {
        const float timeToFinish = remainingProgressRatio / progressPerSecond;
        deficitCatchupRate = particleDeficit / timeToFinish / frameFrequency;
      }

      // Combine: normal rate + deficit catch-up spread over remaining time
      particlesToDepopPerIteration = normalDepopRate + deficitCatchupRate;
    }
    else if (particlesLeft > 0.0f && progress >= 1.0f)
    {
      // Progress reached 1.0 but particles remain; depop everything immediately
      particlesToDepopPerIteration = particlesLeft;
    }
    else
    {
      particlesToDepopPerIteration = 0.0;
    }

    // Safeguard: cap depop rate to prevent overshoot
    const float maxParticlesToDepop = particlesLeft;
    if (particlesToDepopPerIteration > maxParticlesToDepop)
      particlesToDepopPerIteration = maxParticlesToDepop;
  }

protected:
  static bool recycle_particules_if_too_far(const Particle& p)
  {
    static constexpr float bounds = 3.0;
    return p.z_mm > LampTy::maxWidth * bounds or p.z_mm < -(LampTy::lampHeight_mm + LampTy::maxWidth * bounds);
  }

  bool depop_one_particles(auto& ctx)
  {
    auto& buffer = ctx.lamp.template getTempBuffer<MaskBuffId>();
    const auto bufferSize = ctx.lamp.ledCount;

    // Select a pixel to drop
    uint32_t pixelId;
    if (not DropMode::template depop_one<decltype(ctx), MaskBuffId, minMaskValue>(ctx, pixelId) or pixelId < 0 or
        pixelId >= bufferSize)
    {
      bsp::lampda_print("Error: Invalid pixel ID from drop mode");
      return false;
    }

    // drop this pixel
    buffer[pixelId] = minMaskValue;
    particlesDropped += 1;

    // spawn a new particle
    const bool isCreated = particuleSystem.init_deferred_particules(1, [pixelId](size_t) {
      return pixelId;
    });
    // Particle spawn can fail, it's only visual so ok

    return true;
  }

private:
  static constexpr float gracePeriod = 0.99; /// Added offset from the real timer to let all particle disappear
  //
  float latestProgress = -1.0;
  uint32_t latestProgressTime = 0;

  float particlesToDepopPerIteration = 0.0; /// numbers of particles that will fall per loop call
  float depopRate = 0.0;                    /// accumulator rate
  size_t particlesDropped = 0;              /// keep track of the particles that already fell or are falling

  /// All animations share this particle system !
  inline static auto particuleSystem = modes::get_shared_particle_system();
};

} // namespace fadeout
} // namespace lampda::modes::anims

#endif

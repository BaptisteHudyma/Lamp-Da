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
 * \brief Make an animation disapear with gravity
 * \param MaskBuffId Index of the buffer that will contain the mask values for the dropped leds
 */
template<uint8_t MaskBuffId> struct GravityDissolve
{
  void reset(auto& ctx)
  {
    // set all values to 1
    ctx.lamp.template fillTempBuffer<MaskBuffId>(1);
    // reset particles
    particuleSystem.reset();
    particuleSystem.set_max_particle_count(20);

    latestProgress = -1;
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

    // no updats if no particles
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
  void update_depop_rate(auto& ctx, const float progress)
  {
    const float updateDuration = static_cast<float>((ctx.lamp.now - latestProgressTime) / 1000.0);
    const float progressPerSecond = (progress - latestProgress) / updateDuration;

    bool isValidCall = latestProgress >= 0 and updateDuration > 0;

    // cancel the particle spawn
    if (progress <= 0.0)
    {
      particlesToDepopPerIteration = 0.0;
      latestProgress = -1;
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
    const float FramesFrequency = static_cast<float>(ctx.lamp.frameDurationMs / 1000.0);

    const float particlesLeft = particleMax - particlesDropped;
    const float trueProgress = 1.0 - particlesLeft / static_cast<float>(particleMax);
    const float trueProgressDiff = progress - trueProgress;
    // try to correct the update ratio by the lag or advance
    const float correctedParticlesLeft = particleMax * (1.0 + trueProgressDiff);

    // particles to depop per second is progress rate per second * particles left
    particlesToDepopPerIteration = progressPerSecond * correctedParticlesLeft * FramesFrequency;
  }

protected:
  static bool recycle_particules_if_too_far(const Particle& p)
  {
    static constexpr float bounds = 3.0;
    return p.z_mm > LampTy::maxWidth * bounds or p.z_mm < -(LampTy::lampHeight_mm + LampTy::maxWidth * bounds);
  }

  bool depop_one_particles(auto& ctx)
  {
    bool isDepoped = false;
    // progress goes from 0 to 1
    // lamp turns off when 1 is reached
    auto& buffer = ctx.lamp.template getTempBuffer<MaskBuffId>();

    // check lines by lines, keeping the first one with pixels left
    for (int16_t y = ctx.lamp.maxHeight; y >= 0; y--)
    {
      // search for the first line with pixels sets, from bottom to top
      uint8_t pixelSetCount = 0;
      for (size_t x = 0; x <= ctx.lamp.maxWidth; x++)
      {
        const bool isPixelSet = buffer[modes::to_strip(x, y)] != 0;
        // check that at leat one pixel is still set
        if (isPixelSet)
          pixelSetCount += 1;
      }

      // found the first line with pixels left
      if (pixelSetCount > 0)
      {
        // get a random led index in this
        uint8_t selectedLed = lmpd_map<uint8_t>(rand(), 0, RAND_MAX, 0, pixelSetCount);
        for (size_t x = 0; x <= ctx.lamp.maxWidth; x++)
        {
          const auto& pixelId = modes::to_strip(x, y);
          const bool isPixelSet = buffer[pixelId] != 0;
          if (isPixelSet)
          {
            if (selectedLed == 0)
            {
              // drop this pixel
              buffer[pixelId] = 0;
              particlesDropped += 1;
              // spawn a new particle
              particuleSystem.init_deferred_particules(1, [pixelId](size_t) {
                return pixelId;
              });

              isDepoped = true;
              break;
            }
            selectedLed -= 1;
          }
        }
        break;
      }
    }
    if (not isDepoped)
    {
      platform::lampda_print("Error: Could not depop particle");
      return false;
    }
    return true;
  }

private:
  //
  float latestProgress = -1.0;
  uint32_t latestProgressTime = 0;

  float particlesToDepopPerIteration = 0.0; /// numbers of particles that will fall per loop call
  float depopRate = 0.0;                    /// accumulator rate
  size_t particlesDropped = 0;              /// keep track of the particles that already fell or are falling
  modes::ParticleSystem particuleSystem = modes::ParticleSystem();
};

} // namespace fadeout
} // namespace lampda::modes::anims

#endif

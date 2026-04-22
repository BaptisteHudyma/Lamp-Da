#ifndef BRIGHTNESS_MODES_H
#define BRIGHTNESS_MODES_H

/// @file brightness_modes.hpp

#include "src/system/ext/random8.h"
#include "src/system/ext/math8.h"

#include "src/system/utils/utils.h"

namespace lampda::modes::brightness {
/// Basic lightning mode (does nothing, brightness may be adjusted)
struct StaticLightMode : public BasicMode
{
  static constexpr void loop(auto& ctx)
  {
    if constexpr (ctx.lamp.flavor == hardware::LampTypes::indexable)
      ctx.lamp.fill(colors::Candle);
  }
};

/// One mode, alone in its group, for static lightning
using StaticLightOnly = GroupFor<StaticLightMode>;

/// Pulse N times then pause one second
template<size_t N> struct Pulses : public BasicMode
{
  static constexpr uint32_t pulseSizeMs = 100;                     ///< lenght of a pulse, in milliseconds
  static constexpr uint32_t periodMs = 1000 + N * pulseSizeMs * 2; ///< period of the animation, in milliseconds
  static constexpr brightness_t minScaling = 50;                   ///< scaling of the brightness of a pulse
  static constexpr float scaleFactor = 1.40;                       ///< scale of the brigthness

  static constexpr void loop(auto& ctx)
  {
    auto& lamp = ctx.lamp;

    brightness_t base = lamp.getSavedBrightness();
    brightness_t scaled = std::max<brightness_t>(base * scaleFactor, base + minScaling);

    // overflow check
    if (scaled > lamp.getMaxBrightness())
      lamp.jumpBrightness(lamp.getMaxBrightness() / scaleFactor);

    auto period = lamp.now % periodMs;
    // first part of the anim : pulse
    if (period < pulseSizeMs * N * 2)
    {
      uint32_t flipflop = period / pulseSizeMs;
      if (flipflop % 2)
        lamp.restoreBrightness();
      else
        lamp.tempBrightness(scaled);
    }
    // second part: just stay at the saved brightness level
    else
      lamp.restoreBrightness();
  }
};

/// Based on the candle animation from Anduril.
/// candle-mode.c: Candle mode for Anduril.
/// Copyright (C) 2017-2023 Selene ToyKeeper.
/// SPDX-License-Identifier: GPL-3.0-or-later.
struct Candle : public BasicMode
{
  static const uint8_t CANDLE_AMPLITUDE = 33;      ///< amplitude of the candle light
  static const uint8_t CANDLE_WAVE1_MAXDEPTH = 30; ///< Wave depth parameters
  static const uint8_t CANDLE_WAVE2_MAXDEPTH = 45; ///< Wave depth parameters
  static const uint8_t CANDLE_WAVE3_MAXDEPTH = 25; ///< Wave depth parameters

  struct StateTy
  {
    uint8_t candle_wave1;       ///< triangular wave 1 amplitude
    uint8_t candle_wave2;       ///< triangular wave 2 amplitude
    uint8_t candle_wave3;       ///< triangular wave 3 amplitude
    uint8_t candle_wave2_speed; ///< second wave speed (candle flicker)

    uint8_t candle_wave1_depth; ///< Control the wave 1 depth
    uint8_t candle_wave2_depth; ///< Control the wave 2 depth
    uint8_t candle_wave3_depth; ///< Control the wave 3 depth
  };

  static void on_enter_mode(auto& ctx)
  {
    ctx.state.candle_wave1 = 0;
    ctx.state.candle_wave2 = 0;
    ctx.state.candle_wave3 = 0;
    ctx.state.candle_wave2_speed = 0;

    ctx.state.candle_wave1_depth = CANDLE_WAVE1_MAXDEPTH * CANDLE_AMPLITUDE / 100;
    ctx.state.candle_wave2_depth = CANDLE_WAVE2_MAXDEPTH * CANDLE_AMPLITUDE / 100;
    ctx.state.candle_wave3_depth = CANDLE_WAVE3_MAXDEPTH * CANDLE_AMPLITUDE / 100;
  }

  static void loop(auto& ctx)
  {
    auto& lamp = ctx.lamp;

    const auto maxBrightness = lamp.getMaxBrightness();
    const float brightnessCorrection = maxBrightness / 255.0;

    // limit max brightness
    const brightness_t base = std::min<brightness_t>(
            lamp.getSavedBrightness(),
            maxBrightness - std::min<int>(maxBrightness, (CANDLE_AMPLITUDE + 15) * brightnessCorrection));

    // 3-oscillator synth for a relatively organic pattern
    const uint8_t add = ((triwave8(ctx.state.candle_wave1) * ctx.state.candle_wave1_depth) >> 8) +
                        ((triwave8(ctx.state.candle_wave2) * ctx.state.candle_wave2_depth) >> 8) +
                        ((triwave8(ctx.state.candle_wave3) * ctx.state.candle_wave3_depth) >> 8);
    const brightness_t brightness = base + add * brightnessCorrection;

    // update brightness
    lamp.setBrightness(brightness);

    // update candle parameters
    // wave1: slow random LFO
    // TODO: make wave slower and more erratic?
    ctx.state.candle_wave1 += random8() & 1;
    // wave2: medium-speed erratic LFO
    ctx.state.candle_wave2 += ctx.state.candle_wave2_speed;
    // wave3: erratic fast wave
    ctx.state.candle_wave3 += random8() % 37;
    // S&H on wave2 frequency to make it more erratic
    if ((random8() & 0b00111111) == 0)
      ctx.state.candle_wave2_speed = random8() % 13;
    // downward sawtooth on wave2 depth to simulate stabilizing
    if ((ctx.state.candle_wave2_depth > 0) && ((random8() & 0b00111111) == 0))
      ctx.state.candle_wave2_depth--;
    // random sawtooth retrigger
    if (random8() == 0)
    {
      // random amplitude
      // candle_wave2_depth = 2 + (random8() % ((CANDLE_WAVE2_MAXDEPTH * CANDLE_AMPLITUDE / 100) - 2));
      ctx.state.candle_wave2_depth = random8() % (CANDLE_WAVE2_MAXDEPTH * CANDLE_AMPLITUDE / 100);
      // candle_wave3_depth = 5;
      ctx.state.candle_wave2 = 0;
    }
    // downward sawtooth on wave3 depth to simulate stabilizing
    if ((ctx.state.candle_wave3_depth > 2) && ((random8() & 0b00011111) == 0))
      ctx.state.candle_wave3_depth--;
    if ((random8() & 0b01111111) == 0)
      // random amplitude
      // candle_wave3_depth = 2 + (random8() % ((CANDLE_WAVE3_MAXDEPTH * CANDLE_AMPLITUDE / 100) - 2));
      ctx.state.candle_wave3_depth = random8() % (CANDLE_WAVE3_MAXDEPTH * CANDLE_AMPLITUDE / 100);
  }
};

/**
 * \brief Very fast on pulse followed by longer off pulses. Make a stroboscopic effect.
 */
struct StroboscopeMode : public BasicMode
{
  /// regulate stroboscopic speed
  static constexpr bool hasCustomRamp = true;
  /// maximum allowed stroboscopic frequency in Hertz
  static constexpr uint32_t stroboMaxFreq = 1000 * (1 / 30.0f);
  /// minimum allowed stroboscopic frequency in hertz
  static constexpr uint32_t stroboMinFreq = 1000 * (1 / 7.0f);

  struct StateTy
  {
    /// last off pulse set
    uint32_t lastCall;
    /// duration of the off pulse
    uint32_t pulseDuration;
  };

  static void on_enter_mode(auto& ctx)
  {
    ctx.state.lastCall = 0;
    ctx.template set_config_bool<ConfigKeys::rampSaturates>(true);
    ctx.template set_config_bool<ConfigKeys::customRampAnimEffect>(false);

    custom_ramp_update(ctx, ctx.get_active_custom_ramp());
  }

  /// User ramp controls the strob frequency
  static void custom_ramp_update(auto& ctx, uint8_t rampValue)
  {
    ctx.state.pulseDuration = lmpd_map<uint32_t>(rampValue, 0, 255, stroboMinFreq, stroboMaxFreq);
  }

  static void loop(auto& ctx)
  {
    const auto pulseDuration = ctx.state.pulseDuration;

    /// max time of the on pulse, in milliseconds
    static constexpr uint32_t onTime = 5;
    if (ctx.lamp.now - ctx.state.lastCall >= pulseDuration)
    {
      ctx.blip(pulseDuration - onTime);
      ctx.state.lastCall = ctx.lamp.now;
    }
  }
};

/**
 * \brief Emulate lightning in the distance
 * Inspired by Anduril implementation
 */
struct LightningMode : public BasicMode
{
  /// Hint manager that we need a brightness callback
  static constexpr bool hasBrightCallback = true;

  struct StateTy
  {
    /// prevent updates when the user ramp is rolling
    uint32_t lastBrightnessHandleCall;
    /// Is off and waiting to turn on
    bool isWaitingTurnOn;
    /// last brigthness before going dark
    brightness_t lastBrightness;
    /// Brigthness ramp down index
    uint32_t stepdown;
  };

  static void on_enter_mode(auto& ctx)
  { //
    ctx.state.lastBrightnessHandleCall = 0;
    ctx.state.isWaitingTurnOn = true;
    ctx.state.lastBrightness = ctx.lamp.getSavedBrightness();
    ctx.state.stepdown = 0;
  }

  /// If user update the brighness, do not update the animation
  static void brightness_update(auto& ctx, brightness_t brightness)
  {
    auto& lamp = ctx.lamp;
    lamp.cancel_blip();
    ctx.state.lastBrightnessHandleCall = lamp.now;
  }

  static void loop(auto& ctx)
  {
    // update brightness
    auto& lamp = ctx.lamp;

    // no update when ramp is active
    if (lamp.now - ctx.state.lastBrightnessHandleCall < 100)
      return;

    // decrease brightness

    if (not ctx.is_bliping())
    {
      brightness_t currentBrightness = ctx.state.lastBrightness;

      // lampe is still on, decrease brightess
      if (ctx.state.isWaitingTurnOn)
      {
        const brightness_t savedBrightness = lamp.getSavedBrightness();

        // from anduril
        uint8_t n_brightness = 1 << (random8() % 7); // 1, 2, 4, 8, 16, 32, 64
        n_brightness += 1 << (random8() % 5);        // 2 to 80 now
        n_brightness += random8() % n_brightness;    // 2 to 159 now (w/ low bias)

        const brightness_t newBrightness = std::min<brightness_t>(
                savedBrightness, lmpd_map<brightness_t>(n_brightness, 2, 159, 0, savedBrightness));
        // prepare next brightness
        lamp.tempBrightness(newBrightness);
        ctx.state.lastBrightness = newBrightness;

        ctx.state.stepdown = std::max<uint32_t>(1, newBrightness >> 4);

        ctx.state.isWaitingTurnOn = false;
      }
      else if (currentBrightness > ctx.state.stepdown)
      {
        // bleed flash down
        if (ctx.state.stepdown > 0)
        {
          if (random8() & 0x11 != 0)
          {
            currentBrightness >>= 1;
          }
          else
          {
            currentBrightness -= ctx.state.stepdown;
            ctx.state.lastBrightness = currentBrightness;
          }
        }
        else
        {
          currentBrightness = 0;
          ctx.state.lastBrightness = 0;
        }

        lamp.tempBrightness(currentBrightness);
      }
      else
      {
        currentBrightness = 0;

        // from anduril
        // for a random amount of time between 1ms and 8192ms
        uint32_t rand_time = 1 << (random8() % 13);
        rand_time += random8() % rand_time;

        // turn off for a time
        ctx.blip(rand_time);
        ctx.state.isWaitingTurnOn = true;
      }
    }
    // else: we are in a waiting time
  }
};

/// Group for calm modes
using CalmGroup = GroupFor<Candle, LightningMode>;
/// Group for flashing modes
using FlashesGroup = GroupFor<StroboscopeMode, Pulses<1>, Pulses<2>>;

} // namespace lampda::modes::brightness

#endif

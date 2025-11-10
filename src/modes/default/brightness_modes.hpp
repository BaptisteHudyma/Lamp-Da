#ifndef BRIGHTNESS_MODES_H
#define BRIGHTNESS_MODES_H

#include "src/system/ext/random8.h"
#include "src/system/ext/math8.h"

#include "src/system/utils/utils.h"

namespace modes::brightness {
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

/// Simple pulse every second
struct OnePulse : public BasicMode
{
  static constexpr uint32_t periodMs = 1000;
  static constexpr uint32_t pulseSizeMs = 100;
  static constexpr brightness_t minScaling = 50;
  static constexpr float scaleFactor = 1.40;

  static constexpr void loop(auto& ctx)
  {
    auto& lamp = ctx.lamp;

    brightness_t base = lamp.getSavedBrightness();
    brightness_t scaled = max<brightness_t>(base * scaleFactor, base + minScaling);

    if (scaled > lamp.maxBrightness)
      lamp.jumpBrightness(lamp.maxBrightness / scaleFactor);

    auto period = lamp.now % periodMs;
    if (period < pulseSizeMs)
      lamp.tempBrightness(scaled);
    else
      lamp.restoreBrightness();
  }
};

/// Pulse N times then pause one second
template<size_t N> struct ManyPulse : public BasicMode
{
  static constexpr uint32_t pulseSizeMs = 100;
  static constexpr uint32_t periodMs = 1000 + N * pulseSizeMs * 2;
  static constexpr brightness_t minScaling = 50;
  static constexpr float scaleFactor = 1.40;

  static constexpr void loop(auto& ctx)
  {
    auto& lamp = ctx.lamp;

    brightness_t base = lamp.getSavedBrightness();
    brightness_t scaled = max<brightness_t>(base * scaleFactor, base + minScaling);

    if (scaled > lamp.maxBrightness)
      lamp.jumpBrightness(lamp.maxBrightness / scaleFactor);

    auto period = lamp.now % periodMs;
    if (period < pulseSizeMs * N * 2)
    {
      uint32_t flipflop = period / pulseSizeMs;
      if (flipflop % 2)
        lamp.restoreBrightness();
      else
        lamp.tempBrightness(scaled);
    }
    else
      lamp.restoreBrightness();
  }
};

// Based on the candle animation from Anduril
// candle-mode.c: Candle mode for Anduril.
// Copyright (C) 2017-2023 Selene ToyKeeper
// SPDX-License-Identifier: GPL-3.0-or-later
struct Candle : public BasicMode
{
  static const uint8_t CANDLE_AMPLITUDE = 33;
  static const uint8_t CANDLE_WAVE1_MAXDEPTH = 30;
  static const uint8_t CANDLE_WAVE2_MAXDEPTH = 45;
  static const uint8_t CANDLE_WAVE3_MAXDEPTH = 25;

  struct StateTy
  {
    uint8_t candle_wave1;
    uint8_t candle_wave2;
    uint8_t candle_wave3;
    uint8_t candle_wave2_speed;

    uint8_t candle_wave1_depth;
    uint8_t candle_wave2_depth;
    uint8_t candle_wave3_depth;
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
    static constexpr float brightnessCorrection = maxBrightness / 255.0;

    auto& lamp = ctx.lamp;

    // limit max brightness
    const brightness_t base = min<brightness_t>(lamp.getSavedBrightness(),
                                                maxBrightness - (CANDLE_AMPLITUDE + 15) * brightnessCorrection);

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

struct StroboscopeMode : public BasicMode
{
  /// regulate stroboscopic speed
  static constexpr bool hasCustomRamp = true;

  static constexpr uint32_t stroboMaxFreq = 1000 * (1 / 30.0f);
  static constexpr uint32_t stroboMinFreq = 1000 * (1 / 7.0f);

  struct StateTy
  {
    uint32_t lastCall;
    uint32_t pulseDuration;
  };

  static void on_enter_mode(auto& ctx)
  {
    ctx.state.lastCall = 0;
    ctx.template set_config_bool<ConfigKeys::rampSaturates>(true);
    ctx.template set_config_bool<ConfigKeys::customRampAnimEffect>(false);

    custom_ramp_update(ctx, ctx.get_active_custom_ramp());
  }

  static void custom_ramp_update(auto& ctx, uint8_t rampValue)
  {
    ctx.state.pulseDuration = lmpd_map<uint32_t>(rampValue, 0, 255, stroboMinFreq, stroboMaxFreq);
  }

  static void loop(auto& ctx)
  {
    const auto pulseDuration = ctx.state.pulseDuration;

    // no stroboscope for indexable strip
    static constexpr uint32_t onTime = 5;
    if (ctx.lamp.now - ctx.state.lastCall >= pulseDuration)
    {
      ctx.blip(pulseDuration - onTime);
      ctx.state.lastCall = ctx.lamp.now;
    }
  }
};

using CalmGroup = GroupFor<Candle>;
using FlashesGroup = GroupFor<StroboscopeMode, OnePulse, ManyPulse<2>>;

} // namespace modes::brightness

#endif

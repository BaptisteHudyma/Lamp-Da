#include "button_bindings.h"

#include "src/system/bsp/filesystem.h"
#include "src/system/bsp/text_out.h"

#include "src/system/utils/utils.h"

#include <array>
#include <cstring>

namespace lampda {
namespace logic {
namespace button_bindings {

/// Modes store their named keys on 31 bits, and their indexed keys above. Stay out of the indexed key space
static constexpr uint32_t storageKey = utils::hash("btnBindings") & 0x7fffffffu;

/// Number of stored bits per behavior, enough for maxClicks
static constexpr size_t storedBitsPerBehavior = 4;
static_assert(maxClicks < (1 << storedBitsPerBehavior), "Button bindings do not fit in their stored bits anymore");
static_assert(behaviorCount * storedBitsPerBehavior <= 32, "Button bindings do not fit in a stored value anymore");

static constexpr std::array<uint8_t, behaviorCount> defaultClicks = {4, 6, 5, 3, 7, 6};
static constexpr std::array<const char*, behaviorCount> names = {
        "favorite", "nav", "sunset", "ramp", "indicator", "scroll"};

static std::array<uint8_t, behaviorCount> currentClicks = defaultClicks;

static constexpr size_t to_index(const Behavior behavior) { return static_cast<size_t>(behavior); }

static constexpr bool has_click_action(const Behavior behavior)
{
  return behavior == Behavior::Favorite or behavior == Behavior::QuickNavigation or behavior == Behavior::Indicator;
}

static constexpr bool has_hold_action(const Behavior behavior) { return behavior != Behavior::QuickNavigation; }

/// Two behaviors using the same action cannot share a click count
static constexpr bool share_an_action(const Behavior a, const Behavior b)
{
  return (has_click_action(a) and has_click_action(b)) or (has_hold_action(a) and has_hold_action(b));
}

static bool is_allowed(const Behavior behavior, const uint8_t clicks)
{
  return clicks >= get_min_clicks(behavior) and clicks <= maxClicks;
}

/// Check that every behavior of this lamp is on an allowed click count, and alone on its actions
static bool is_coherent(const std::array<uint8_t, behaviorCount>& clicks)
{
  for (size_t i = 0; i < behaviorCount; ++i)
  {
    if (not is_supported(static_cast<Behavior>(i)))
      continue;
    if (not is_allowed(static_cast<Behavior>(i), clicks[i]))
      return false;

    for (size_t j = i + 1; j < behaviorCount; ++j)
    {
      const Behavior other = static_cast<Behavior>(j);
      if (is_supported(other) and clicks[i] == clicks[j] and share_an_action(static_cast<Behavior>(i), other))
        return false;
    }
  }
  return true;
}

uint8_t get_min_clicks(const Behavior behavior)
{
#ifdef LMBD_LAMP_TYPE__CCT
  // 3 and 4 click+hold are the CCT color ramps
  return 5;
#else
  // 3 clicks is always the group navigation, only 3 click+hold is free
  return has_click_action(behavior) ? 4 : 3;
#endif
}

bool is_supported(const Behavior behavior)
{
  switch (behavior)
  {
    // handled by the system, for all the lamp types
    case Behavior::Indicator:
      return true;
#ifdef LMBD_LAMP_TYPE__INDEXABLE
    case Behavior::Favorite:
    case Behavior::Scroll:
      return true;
#endif
#ifndef LMBD_LAMP_TYPE__CCT
    case Behavior::QuickNavigation:
    case Behavior::Sunset:
    case Behavior::CustomRamp:
      return true;
#endif
    default:
      return false;
  }
}

Behavior get_click_behavior(const uint8_t clicks)
{
  for (size_t i = 0; i < behaviorCount; ++i)
  {
    const Behavior behavior = static_cast<Behavior>(i);
    if (currentClicks[i] == clicks and has_click_action(behavior) and is_supported(behavior))
      return behavior;
  }
  return Behavior::None;
}

Behavior get_hold_behavior(const uint8_t clicks)
{
  for (size_t i = 0; i < behaviorCount; ++i)
  {
    const Behavior behavior = static_cast<Behavior>(i);
    if (currentClicks[i] == clicks and has_hold_action(behavior) and is_supported(behavior))
      return behavior;
  }
  return Behavior::None;
}

uint8_t get_clicks(const Behavior behavior) { return is_supported(behavior) ? currentClicks[to_index(behavior)] : 0; }

const char* get_name(const Behavior behavior)
{
  return behavior == Behavior::None ? "none" : names[to_index(behavior)];
}

Behavior get_behavior_from_name(const char* name)
{
  for (size_t i = 0; i < behaviorCount; ++i)
  {
    if (strcmp(names[i], name) == 0)
      return static_cast<Behavior>(i);
  }
  return Behavior::None;
}

SetResult set_binding(const Behavior behavior, const uint8_t clicks)
{
  if (not is_supported(behavior))
    return SetResult::UnsupportedBehavior;
  if (not is_allowed(behavior, clicks))
    return SetResult::ReservedClicks;

  // the behaviors in conflict take the click count that is now free
  const uint8_t freedClicks = currentClicks[to_index(behavior)];
  std::array<uint8_t, behaviorCount> candidate = currentClicks;
  for (size_t i = 0; i < behaviorCount; ++i)
  {
    const Behavior other = static_cast<Behavior>(i);
    if (other != behavior and is_supported(other) and currentClicks[i] == clicks and share_an_action(behavior, other))
      candidate[i] = freedClicks;
  }
  candidate[to_index(behavior)] = clicks;

  // what was moved can collide with what stayed on the freed click count
  if (not is_coherent(candidate))
    return SetResult::SwapNotPossible;

  currentClicks = candidate;
  return SetResult::Success;
}

void reset_to_defaults() { currentClicks = defaultClicks; }

void load_from_memory()
{
  reset_to_defaults();

  uint32_t stored = 0;
  if (not bsp::filesystem::user::get_value(storageKey, stored))
    return;

  std::array<uint8_t, behaviorCount> loaded = defaultClicks;
  for (size_t i = 0; i < behaviorCount; ++i)
    loaded[i] = static_cast<uint8_t>((stored >> (storedBitsPerBehavior * i)) & ((1u << storedBitsPerBehavior) - 1u));

  if (is_coherent(loaded))
    currentClicks = loaded;
  else
    bsp::lampda_print("Ignored incoherent stored button bindings, using the defaults");
}

void write_to_memory()
{
  uint32_t stored = 0;
  for (size_t i = 0; i < behaviorCount; ++i)
    stored |= static_cast<uint32_t>(currentClicks[i]) << (storedBitsPerBehavior * i);

  bsp::filesystem::user::set_value(storageKey, stored);
}

} // namespace button_bindings
} // namespace logic
} // namespace lampda

#ifndef CONTEXT_TYPE_H
#define CONTEXT_TYPE_H

#include <cstdint>
#include <type_traits>

/** \file context_type.h
 *  \brief ContextTy and associated definitions
 **/

#include "tools.h"

namespace modes {

/// Bind provided context to another modes::BasicMode
///
/// \param[in] ctx Local context to bind to \p NewLocalMode
/// \return A new context instance bound to the provided modes::BasicMode
template <typename NewLocalMode, typename CtxTy>
static auto context_as(CtxTy& ctx) {
  return ctx.template context_as<NewLocalMode>();
}

/// Local context exposing system features to active BasicMode
template <typename LocalMode, typename ModeManager>
struct ContextTy {
  friend ModeManager;

  using LocalBasicMode = LocalMode;
  using ModeManagerTy = ModeManager;

  //
  // constructors
  //

  /// Get the same context, but for another mode, see modes::context_as()
  template <typename NewLocalMode>
  auto LMBD_INLINE context_as() {
    return ContextTy<NewLocalMode, ModeManagerTy>(modeManager);
  }

  /// \private Use ModeManagerTy::get_context() to construct context
  ContextTy(ModeManagerTy& modeManager)
      : strip{modeManager.strip},
        modeHasChanged{modeManager.modeHasChanged},
        groupHasChanged{modeManager.groupHasChanged},
        modeManager{modeManager} { }

  ContextTy() = delete; ///< \private
  ContextTy(const ContextTy&) = delete; ///< \private
  ContextTy& operator=(const ContextTy&) = delete; ///< \private

  //
  // manager calls
  //

  /// \private Jump to next group
  auto LMBD_INLINE next_group() {
    if constexpr (LocalBasicMode::isGroupManager) {
      LocalBasicMode::next_group(*this);
    }
  }

  /// \private Jump to next mode
  auto LMBD_INLINE next_mode() {
    if constexpr (LocalBasicMode::isModeManager) {
      LocalBasicMode::next_mode(*this);
    }
  }

  /// \private Reset active mode
  auto LMBD_INLINE reset_mode() {
    if constexpr (LocalBasicMode::isModeManager) {
      LocalBasicMode::reset_mode(*this);
    }
  }

  //
  // getters / setters for activeIndex
  //

  /// (getter) Get active group index
  uint8_t get_active_group(uint8_t maxValueWrap = 255) const {
    uint8_t groupIndex = modeManager.activeIndex.groupIndex;
    return groupIndex >= maxValueWrap ? 0 : groupIndex;
  }

  /// (setter) Set active group index
  uint8_t set_active_group(uint8_t value, uint8_t maxValueWrap = 255) {
    if (value >= maxValueWrap) value = 0;
    modeManager.activeIndex.groupIndex = value;
    return value;
  }

  /// (getter) Get active mode index (as numbered in local group)
  uint8_t get_active_mode(uint8_t maxValueWrap = 255) const {
    uint8_t modeIndex = modeManager.activeIndex.modeIndex;
    return modeIndex >= maxValueWrap ? 0 : modeIndex;
  }

  /// (setter) Set active mode index (as numbered in local group)
  uint8_t set_active_mode(uint8_t value, uint8_t maxValueWrap = 255) {
    if (value >= maxValueWrap) value = 0;
    modeManager.activeIndex.modeIndex = value;
    return value;
  }

  /** \brief (getter) Get active custom ramp value (as configured by user)
   *
   * \see BasicMode::custom_ramp_update()
   */
  uint8_t get_active_custom_ramp(uint8_t maxValueWrap = 255) const {
    uint8_t rampIndex = modeManager.activeIndex.rampIndex;
    return rampIndex >= maxValueWrap ? 0 : rampIndex;
  }

  /** \brief (setter) Set active custom ramp value (overrides user choice)
   *
   * \see BasicMode::custom_ramp_update()
   */
  uint8_t set_active_custom_ramp(uint8_t value, uint8_t maxValueWrap = 255) {
    if (value >= maxValueWrap) value = 0;
    modeManager.activeIndex.rampIndex = value;
    return value;
  }

  /// (getter) Get active custom index (to be recalled when jumping favorites)
  uint8_t get_active_custom_index(uint8_t maxValueWrap = 255) const {
    uint8_t customIndex = modeManager.activeIndex.customIndex;
    return customIndex >= maxValueWrap ? 0 : customIndex;
  }

  /// (setter) Set active custom index (to be recalled when jumping favorites)
  uint8_t set_active_custom_index(uint8_t value, uint8_t maxValueWrap = 255) {
    if (value >= maxValueWrap) value = 0;
    modeManager.activeIndex.customIndex = value;
    return value;
  }

  //
  // quick bindings to \p LocalBasicMode
  //

  /// Binds to local BasicMode::loop()
  void LMBD_INLINE loop() {
    if constexpr (LocalBasicMode::simpleMode) {
      LocalBasicMode::loop(this->strip);
    } else {
      LocalBasicMode::loop(*this);
    }
  }

  /// Binds to local BasicMode::reset()
  void LMBD_INLINE reset() {
    LocalBasicMode::reset(*this);
  }

  /// Binds to local BasicMode::brightness_update()
  void LMBD_INLINE brightness_update(uint8_t brightness) {
    if constexpr (LocalBasicMode::hasBrightCallback) {
      LocalBasicMode::brightness_update(*this, brightness);
    }
  }

  /// Binds to local BasicMode::custom_ramp_update()
  void LMBD_INLINE custom_ramp_update(uint8_t rampValue) {
    if constexpr (LocalBasicMode::hasCustomRamp) {
      LocalBasicMode::custom_ramp_update(*this, rampValue);
    }
  }

  /// Binds to local BasicMode::custom_click()
  bool LMBD_INLINE custom_click(uint8_t nbClick) {
    if constexpr (LocalBasicMode::hasButtonCustomUI) {
      return LocalBasicMode::custom_click(*this, nbClick);
    } else {
      return false;
    }
  }

  /// Binds to local BasicMode::custom_hold()
  bool LMBD_INLINE custom_hold(uint8_t nbClickAndHold,
                               bool isEndOfHoldEvent,
                               uint32_t holdDuration) {
    if constexpr (LocalBasicMode::hasButtonCustomUI) {
      return LocalBasicMode::custom_hold(*this,
                                         nbClickAndHold,
                                         isEndOfHoldEvent,
                                         holdDuration);
    } else {
      return false;
    }
  }

  /// Binds to local BasicMode::power_on_sequence()
  void LMBD_INLINE power_on_sequence() {
    if constexpr (LocalBasicMode::hasSystemCallbacks) {
      LocalBasicMode::power_on_sequence(*this);
    }
  }

  /// Binds to local BasicMode::power_off_sequence()
  void LMBD_INLINE power_off_sequence() {
    if constexpr (LocalBasicMode::hasSystemCallbacks) {
      LocalBasicMode::power_off_sequence(*this);
    }
  }

  /// Binds to local BasicMode::write_parameters()
  void LMBD_INLINE write_parameters() {
    if constexpr (LocalBasicMode::hasSystemCallbacks) {
      LocalBasicMode::write_parameters(*this);
    }
  }

  /// Binds to local BasicMode::read_parameters()
  void LMBD_INLINE read_parameters() {
    if constexpr (LocalBasicMode::hasSystemCallbacks) {
      LocalBasicMode::read_parameters(*this);
    }
  }

  /// Returns BasicMode::requireUserThread
  bool LMBD_INLINE should_spawn_thread() {
    return LocalBasicMode::requireUserThread;
  }

  /// Binds to local BasicMode::user_thread()
  void LMBD_INLINE user_thread() {
    if constexpr (LocalBasicMode::requireUserThread) {
      LocalBasicMode::user_thread(*this);
    }
  }

  //
  // context members for direct access
  //

  LedStrip& strip;

  bool modeHasChanged; ///< (compatibility) Prefer using BasicMode::reset()
  bool groupHasChanged; ///< (compatibility) Prefer using BasicMode::reset()
private:
  ModeManagerTy& modeManager;
};

} // namespace modes

#endif

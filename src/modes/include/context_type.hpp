#ifndef CONTEXT_TYPE_H
#define CONTEXT_TYPE_H

/** \file
 *
 *  \brief ContextTy and associated definitions
 **/

#include <cstdint>
#include <type_traits>

#include "src/modes/include/hardware/keystore.hpp"
#include "src/modes/include/hardware/lamp_type.hpp"
#include "src/modes/include/tools.hpp"
#include "src/modes/include/default_config.hpp"

namespace modes {

/// Bind provided context to another modes::BasicMode
///
/// \param[in] ctx Local context to bind to \p NewLocalMode
/// \return A new context instance bound to the provided modes::BasicMode
template<typename NewLocalMode> static auto context_as(auto& ctx) { return ctx.template context_as<NewLocalMode>(); }

/// Local context exposing system features to active BasicMode
template<typename LocalBasicMode, typename ModeManager> struct ContextTy
{
  friend ModeManager;

  using SelfTy = ContextTy<LocalBasicMode, ModeManager>;
  using ModeManagerTy = ModeManager;
  using LocalModeTy = LocalBasicMode;
  using StateTy = StateTyOf<LocalModeTy>;

  /// \private True if LocalModeTy is a BasicMode
  static constexpr bool isMode = is_mode<LocalModeTy>;

  /// \private True if context is root "manager" context
  static constexpr bool isManager = std::is_same_v<LocalModeTy, ModeManagerTy>;

  // useful proxies
  static constexpr bool hasBrightCallback = LocalModeTy::hasBrightCallback;   ///< \private
  static constexpr bool hasSystemCallbacks = LocalModeTy::hasSystemCallbacks; ///< \private
  static constexpr bool requireUserThread = LocalModeTy::requireUserThread;   ///< \private
  static constexpr bool hasCustomRamp = LocalModeTy::hasCustomRamp;           ///< \private
  static constexpr bool hasButtonCustomUI = LocalModeTy::hasButtonCustomUI;   ///< \private

  //
  // constructors
  //

  /// Get the same context, but for another mode, see modes::context_as()
  template<typename NewLocalMode> auto LMBD_INLINE context_as()
  {
    return ContextTy<NewLocalMode, ModeManagerTy>(modeManager);
  }

  /// \private Use ModeManagerTy::get_context() to construct context
  ContextTy(ModeManagerTy& modeManager) :
    state {modeManager.template getStateOf<LocalModeTy>()},
    lamp {modeManager.lamp},
    modeManager {modeManager}
  {
  }

  ContextTy() = delete;                            ///< \private
  ContextTy(const ContextTy&) = delete;            ///< \private
  ContextTy& operator=(const ContextTy&) = delete; ///< \private

  //
  // manager calls
  //

  /// \private Jump to next group
  auto LMBD_INLINE next_group()
  {
    if constexpr (LocalModeTy::isGroupManager)
    {
      LocalModeTy::next_group(*this);
    }
    else
    {
      auto& manager = modeManager.get_context();
      return manager.next_group();
    }
  }

  /// \private Jump to next mode
  auto LMBD_INLINE next_mode()
  {
    if constexpr (LocalModeTy::isModeManager)
    {
      LocalModeTy::next_mode(*this);
    }
    else
    {
      auto& manager = modeManager.get_context();
      return manager.next_mode();
    }
  }

  /// \private Jump to favorite mode
  auto LMBD_INLINE jump_to_favorite(uint8_t which_one = 0)
  {
    if constexpr (LocalModeTy::isModeManager)
    {
      LocalModeTy::jump_to_favorite(*this, which_one);
    }
    else
    {
      auto& manager = modeManager.get_context();
      return manager.jump_to_favorite(which_one);
    }
  }

  /// \private Set active favorite now
  auto LMBD_INLINE set_favorite_now(uint8_t which_one = 0)
  {
    if constexpr (LocalModeTy::isModeManager)
    {
      LocalModeTy::set_favorite_now(*this, which_one);
    }
    else
    {
      auto& manager = modeManager.get_context();
      return manager.set_favorite_now(which_one);
    }
  }

  /// \private Get number of groups available
  auto LMBD_INLINE get_groups_count() { return modeManager.nbGroups; }

  /// \private Get number of modes available
  auto LMBD_INLINE get_modes_count()
  {
    if constexpr (LocalModeTy::isModeManager)
    {
      return modeManager.get_modes_count(*this);
    }
    else
    {
      auto& manager = modeManager.get_context();
      return manager.get_modes_count();
    }
  }

  /// \private Reset active mode
  auto LMBD_INLINE reset_mode()
  {
    if constexpr (LocalModeTy::isModeManager)
    {
      LocalModeTy::reset_mode(*this);
    }
    else
    {
      auto& manager = modeManager.get_context();
      return manager.reset_mode();
    }
  }

  //
  // getters / setters for activeIndex
  //

  /// (getter) Get active group index
  uint8_t LMBD_INLINE get_active_group(uint8_t maxValueWrap = 255) const
  {
    uint8_t groupIndex = modeManager.activeIndex.groupIndex;
    return (groupIndex + 1) > maxValueWrap ? 0 : groupIndex;
  }

  /// (setter) Set active group index
  uint8_t LMBD_INLINE set_active_group(uint8_t value, uint8_t maxValueWrap = 255)
  {
    if (value + 1 > maxValueWrap)
      value = 0;

    // signal that we are quitting the group
    quit_group();
    // switch group
    modeManager.activeIndex.groupIndex = value;
    // signal that we enter a new group
    enter_group();
    return value;
  }

  /// (getter) Get active mode index (as numbered in local group)
  uint8_t LMBD_INLINE get_active_mode(uint8_t maxValueWrap = 255) const
  {
    uint8_t modeIndex = modeManager.activeIndex.modeIndex;
    return (modeIndex + 1) > maxValueWrap ? 0 : modeIndex;
  }

  /// (setter) Set active mode index (as numbered in local group)
  uint8_t LMBD_INLINE set_active_mode(uint8_t value, uint8_t maxValueWrap = 255)
  {
    if (value + 1 > maxValueWrap)
      value = 0;

    // signal that we are quitting the mode
    quit_mode();
    // switch mode
    modeManager.activeIndex.modeIndex = value;
    // signal that we entered a new mode
    enter_mode();
    return value;
  }

  /** \brief (getter) Get active custom ramp value (as configured by user)
   *
   * \see BasicMode::custom_ramp_update()
   */
  uint8_t LMBD_INLINE get_active_custom_ramp() const { return modeManager.activeIndex.rampIndex; }

  /** \brief (setter) Set active custom ramp value (overrides user choice)
   *
   * \see BasicMode::custom_ramp_update()
   */
  uint8_t LMBD_INLINE set_active_custom_ramp(uint8_t value)
  {
    modeManager.activeIndex.rampIndex = value;
    return value;
  }

  /// (getter) Get active custom index (recalled when jumping favorites)
  uint8_t LMBD_INLINE get_active_custom_index() const { return modeManager.activeIndex.customIndex; }

  /// (setter) Set active custom index (recalled when jumping favorites)
  uint8_t LMBD_INLINE set_active_custom_index(uint8_t value)
  {
    modeManager.activeIndex.customIndex = value;
    return value;
  }

  //
  // getters / setters for other properties
  //

  /// (getter) Return current brightness value
  brightness_t LMBD_INLINE get_brightness() { return brightness::get_brightness(); }

  //
  // store
  //

  /// \private Group number where \p LocalBasicMode belongs to (or -1 if absent)
  static constexpr int groupId = details::GroupIdFrom<LocalModeTy, typename ModeManagerTy::AllGroupsTy>;

  /// \private Mode position of \p LocalBasicMode in its group (or -1 if absent)
  static constexpr int modeId = details::ModeIdFrom<LocalModeTy, typename ModeManagerTy::AllGroupsTy, groupId>;

  /// \private Local store will be in general, manager, or private key space ?
  static constexpr auto prefix =
          (isManager ? store::PrefixValues::managerKeys :
                       (isMode ? store::PrefixValues::generalKeys : store::PrefixValues::privateKeys));

  /// \private Store enumeration used by \p LocalModeTy to index its storage
  using StoreEnum = StoreEnumOf<LocalModeTy>;

  /// \private Internal key store implementation used by KeyProxy
  using LocalStore = store::KeyStore<StoreEnum, groupId, modeId, prefix>;

  /// (store) True if \p LocalBasicMode has access to a local key store
  static constexpr bool hasStore = not std::is_same_v<StoreEnum, NoStoreHere>;

  /** \brief (store) Binds a local variable of type \p T to \p key in storage
   * (with automatic read & save)
   *
   * Before using this, you must define in your mode the following:
   *
   * ```
   *    struct MyMode : public modes::BasicMode {
   *
   *      enum class Store : uint16_t {
   *        keyA,
   *        keyB
   *      };
   *
   *      static constexpr uint32_t storeId = modes::store::hash("MyMode");
   *
   *      // ... (other stuff from your mode) ...
   *    };
   * ```
   *
   * You will then be able to bind persistent storage to a local variable:
   *
   * ```
   *    static void loop(auto& ctx) {
   *      float a = 1.5; // default value
   *
   *      // load value for "a" at "keyA" from storage if available
   *      auto storeA = ctx.template storageFor<keyA>(a);
   *
   *      // ... (do some other stuff) ...
   *      a += 1.0;
   *
   *      // no need to explicitly* save "a" (done automatically)
   *    }
   *
   * // *when storeA is destroyed, storage for "a" is updated
   * ```
   *
   * Other advanced use-cases are left undocumented, for experienced users :)
   */
  template<StoreEnum _key, typename T> struct KeyProxy
  {
    static_assert(std::is_same_v<T, std::conditional_t<hasStore, T, NoStoreId>>,
                  "KeyProxy can not be used when no storeId has been defined!");

    /// Enumeration value uniquely identifying where \p local will be stored
    static constexpr StoreEnum key = _key;

    /// Local variable reference bound to KeyProxy
    T& local;

    /// (optional) Set \p local to \p value then write \p key to store
    inline void LMBD_INLINE setValue(T value)
    {
      local = value;
      LocalStore::template setValue<key, T>(value);
    };

    /// (optional) Read \p key from store, return True if \p local was written
    inline bool LMBD_INLINE getValue() { return LocalStore::template getValue<key, T>(local); }

    /// (optional) Read \p key value, if not found, set \p local to \p defVal
    inline void LMBD_INLINE getValue(T defVal) { LocalStore::template getValue<key, T>(local, defVal); }

    /// (optional) Return True if \p key is set in store, False if unknown
    inline bool LMBD_INLINE hasValue() { return LocalStore::template hasValue<key, T>(); }

    //
    // private API
    //

    /// \private Force local to storage, even if done elsewhere automatically
    inline void LMBD_INLINE forceSave() { LocalStore::template setValue<key, T>(local); }

    //
    // constructors
    //

    /// Use ContextTy::storageFor to construct a KeyProxy bound to \p local
    KeyProxy(T& local) : local {local} { getValue(); }

    KeyProxy() = delete;                           ///< \private
    KeyProxy(const KeyProxy&) = delete;            ///< \private
    KeyProxy& operator=(const KeyProxy&) = delete; ///< \private

    /// \private Automagically save value when context is lost
    ~KeyProxy() { forceSave(); }
  };

  /// (store) Get storage KeyProxy for \p key bound to \p local variable
  template<StoreEnum key, typename T = uint32_t>
  static auto LMBD_INLINE storageFor(LMBD_USED T& local) // requires storeId :)
  {
    if constexpr (hasStore)
    {
      return KeyProxy<key, T> {local};
    }
    else
    {
      return NoStoreId {};
    }
  }

  /// (store) Load \p key into \p local if available (single-shot)
  template<StoreEnum key, typename T = uint32_t> static bool LMBD_INLINE storageLoadOnly(LMBD_USED T& local)
  {
    return LocalStore::template getValue<key, T>(local);
  }

  /// (store) Save \p local into \p key storage (single-shot)
  template<StoreEnum key, typename T = uint32_t> static void LMBD_INLINE storageSaveOnly(LMBD_USED T& local)
  {
    return LocalStore::template setValue<key, T>(local);
  }

  //
  // manager config
  //

  /** \brief (setter) Set a configurable boolean to target \p value
   *
   * See modes::ConfigKeys for documentation of configurable booleans.
   */
  template<ConfigKeys key> void LMBD_INLINE set_config_bool(bool value)
  {
    auto ctx = modeManager.get_context();
    switch (key)
    {
      case ConfigKeys::rampSaturates:
        ctx.state.rampHandler.rampSaturates = value;
        break;
      case ConfigKeys::clearStripOnModeChange:
        ctx.state.clearStripOnModeChange = value;
        break;
      case ConfigKeys::customRampAnimEffect:
        ctx.state.rampHandler.animEffect = value;
        break;
      default:
        assert(false && "unsupported config key for booleans!");
        break;
    }
  }

  /** \brief (setter) Set a configurable integer (u32) to target \p value
   *
   * See modes::ConfigKeys for documentation of configurable (u32) integers.
   */
  template<ConfigKeys key> void LMBD_INLINE set_config_u32(uint32_t value)
  {
    auto ctx = modeManager.get_context();
    switch (key)
    {
      case ConfigKeys::customRampStepSpeedMs:
        ctx.state.rampHandler.stepSpeed = value;
        break;
      case ConfigKeys::customRampAnimChoice:
        ctx.state.rampHandler.animChoice = value;
        break;
      default:
        assert(false && "unsupported config key for u32!");
        break;
    }
  }

  //
  // special effects
  //

  /** \brief Skip the few next calls to active mode ``.loop``
   */
  void LMBD_INLINE skipNextFrames(uint8_t count = 1)
  {
    auto ctx = modeManager.get_context();
    ctx.state.skipNextFrameEffect = count;
  }

  /** \brief Skip the first few LEDs update during several frames
   *
   * Next calls to @ref modes::hardware::LampTy.setPixelColor() no longer write
   * on the \p amount first lower LEDs, making them static.
   *
   * See modes::colors::rampColorRing
   *
   */
  void LMBD_INLINE skipFirstLedsForFrames(uint8_t amount, uint8_t count = 1)
  {
    auto ctx = modeManager.get_context();
    ctx.lamp.config.skipFirstLedsForAmount = amount;
    ctx.lamp.config.skipFirstLedsForEffect = count;
  }

  //
  // Binder for group and mode enter/quit
  //

  /// Call when entering a group
  void LMBD_INLINE enter_group()
  {
    if constexpr (LocalModeTy::isModeManager)
    {
      LocalModeTy::enter_group(*this);
    }
    else
    {
      auto& manager = modeManager.get_context();
      return manager.enter_group();
    }
  }

  /// Call when quitting a group
  void LMBD_INLINE quit_group()
  {
    if constexpr (LocalModeTy::isModeManager)
    {
      LocalModeTy::quit_group(*this);
    }
    else
    {
      auto& manager = modeManager.get_context();
      return manager.quit_group();
    }
  }

  /// Call when entering a mode
  void LMBD_INLINE enter_mode()
  {
    if constexpr (LocalModeTy::isModeManager)
    {
      LocalModeTy::enter_mode(*this);
    }
    else
    {
      auto& manager = modeManager.get_context();
      return manager.enter_mode();
    }
  }

  /// Call when quitting a mode
  void LMBD_INLINE quit_mode()
  {
    if constexpr (LocalModeTy::isModeManager)
    {
      LocalModeTy::quit_mode(*this);
    }
    else
    {
      auto& manager = modeManager.get_context();
      return manager.quit_mode();
    }
  }

  //
  // quick bindings to \p LocalModeTy
  //

  /// Binds to local BasicMode::loop()
  void LMBD_INLINE loop()
  {
    LocalModeTy::loop(*this);

    // signal display update every loop
    this->lamp.getLegacyStrip().signal_display();
  }

  /// Binds to local BasicMode::reset()
  void LMBD_INLINE reset()
  {
    if (hasStore)
    {
      LocalStore::template migrateStoreIfNeeded<LocalModeTy::storeId>();
    }
    LocalModeTy::reset(*this);
  }

  /// Binds to local BasicMode::brightness_update()
  void LMBD_INLINE brightness_update(LMBD_USED brightness_t brightness)
  {
    if constexpr (LocalModeTy::hasBrightCallback)
    {
      LocalModeTy::brightness_update(*this, brightness);
    }
  }

  /// Binds to local BasicMode::custom_ramp_update()
  void LMBD_INLINE custom_ramp_update(LMBD_USED uint8_t rampValue)
  {
    if constexpr (LocalModeTy::hasCustomRamp)
    {
      LocalModeTy::custom_ramp_update(*this, rampValue);
    }
  }

  /// Binds to local BasicMode::custom_click()
  bool LMBD_INLINE custom_click(LMBD_USED uint8_t nbClick)
  {
    if constexpr (LocalModeTy::hasButtonCustomUI)
    {
      return LocalModeTy::custom_click(*this, nbClick);
    }

    return false;
  }

  /// Binds to local BasicMode::custom_hold()
  bool LMBD_INLINE custom_hold(LMBD_USED uint8_t nbClickAndHold,
                               LMBD_USED bool isEndOfHoldEvent,
                               LMBD_USED uint32_t holdDuration)
  {
    if constexpr (LocalModeTy::hasButtonCustomUI)
    {
      return LocalModeTy::custom_hold(*this, nbClickAndHold, isEndOfHoldEvent, holdDuration);
    }

    return false;
  }

  /// Binds to local BasicMode::power_on_sequence()
  void LMBD_INLINE power_on_sequence()
  {
    if constexpr (LocalModeTy::hasSystemCallbacks)
    {
      LocalModeTy::power_on_sequence(*this);
    }
  }

  /// Binds to local BasicMode::power_off_sequence()
  void LMBD_INLINE power_off_sequence()
  {
    if constexpr (LocalModeTy::hasSystemCallbacks)
    {
      LocalModeTy::power_off_sequence(*this);
    }
  }

  /// Binds to local BasicMode::write_parameters()
  void LMBD_INLINE write_parameters()
  {
    if constexpr (LocalModeTy::hasSystemCallbacks)
    {
      LocalModeTy::write_parameters(*this);
    }
  }

  /// Binds to local BasicMode::read_parameters()
  void LMBD_INLINE read_parameters()
  {
    if constexpr (LocalModeTy::hasSystemCallbacks)
    {
      LocalModeTy::read_parameters(*this);
    }
  }

  /// Returns BasicMode::requireUserThread
  bool LMBD_INLINE should_spawn_thread() { return LocalModeTy::requireUserThread; }

  /// Binds to local BasicMode::user_thread()
  void LMBD_INLINE user_thread()
  {
    if constexpr (LocalModeTy::requireUserThread)
    {
      LocalModeTy::user_thread(*this);
    }
  }

  //
  // context members for direct access
  //

  hardware::LampTy& lamp; ///< Interact with the lamp hardware
  StateTy& state;         ///< Interact with the current active mode state

private:
  ModeManagerTy& modeManager;
};

} // namespace modes

#endif

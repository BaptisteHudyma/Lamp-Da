#ifndef MODES_DEFAULT_CONFIG_H
#define MODES_DEFAULT_CONFIG_H

namespace modes {

/** \brief Default manager configuration, enables you to customize defaults
 *
 * Implement a custom manager type a sfollow:
 *
 * ```
 *   struct MyCustomConfig : public modes::DefaultManagerConfig {
 *
 *     // default value for clearStripOnModeChange is overridden to True
 *     static constexpr bool defaultClearStripOnModeChange = true;
 *   };
 * ```
 *
 * Then use, instead of the `modes::ManagerFor` helper, the following:
 *
 * ```
 *   // using ManagerTy = modes::ManagerFor<
 *   using ManagerTy = modes::ManagerForConfig<MyCustomConfig,
 *     modes::GroupFor<
 *       mode1,
 *       mode2>,
 *     modes::GroupFor<
 *       modeA,
 *       // ... other modes
 *       modeC>
 *     >;
 * ```
 *
 * See \link ./src/modes/custom/my_custom_config.hpp configuration template
 * \endlink below where you will need to uncomment the values you want to
 * override:
 *
 * \include modes/custom/my_custom_config.hpp
 *
 * See also modes::ConfigKeys for runtime configuration.
 */
struct DefaultManagerConfig {

//
// useful config
//

  /// By default, will custom ramp saturates, or else wrap around?
  static constexpr bool defaultRampSaturates = false;

  /// By default, will strip be cleared between modes, or else do nothing?
  static constexpr bool defaultClearStripOnModeChange = true;

  /// By default, how slow custom ramp changes value (milliseconds)
  static constexpr uint32_t defaultCustomRampStepSpeedMs = 16;

//
// misc config
//

  /// (misc) Override how slow mode & group scroll goes (milliseconds)
  static constexpr uint32_t scrollRampStepSpeedMs = 512;

  /// (misc) Override ramp wait time before starting (milliseconds)
  static constexpr uint32_t rampStartPeriodMs = 128;

  /// (misc) Override default initial active group or mode (by index)
  static constexpr uint8_t initialActiveIndex[4] = {0, 0, 0, 0};

  /// (misc) Override default initial active favorite mode (by index)
  static constexpr uint8_t defaultFavorite[4] = {0, 0, 0, 0};
};

/** \brief Keys to enable modes to change configuration at runtime.
 *
 * During modes::BasicMode::reset() callback, modes can change configuration
 * temporarily, for example to change how the ramp behaves, or how fast it
 * goes.
 *
 * To change a boolean value, use the following:
 *
 * ```
 *   // set value for "rampSaturates" to "True" for active mode
 *   static void reset(auto& ctx) {
 *     ctx.template set_config_bool<ConfigKeys::rampSaturates>(true);
 *   }
 * ```
 *
 * To change an integer value, 32-bit unsigned, use the following:
 *
 * ```
 *   // set value for "customRampStepSpeedMs" to "32" for active mode
 *   static void reset(auto& ctx) {
 *     ctx.template set_config_u32<ConfigKeys::customRampStepSpeedMs>(32);
 *   }
 * ```
 *
 * All these values are reset to their default counterpart before each mode
 * change, see modes::DefaultManagerConfig on how to override these defaults.
 */
enum class ConfigKeys : uint8_t {
  rampSaturates, ///< (bool) Mode saturates on custom ramp, or else wrap?
  clearStripOnModeChange, ///< (bool) Mode clear strip after reset, or else do nothing?
  customRampStepSpeedMs, ///< (u32) Mode time step for incrementing custom ramp (ms)
};

} // namespace modes

#endif

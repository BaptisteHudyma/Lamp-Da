#ifndef MODES_HARDWARE_LAMP_TYPE_HPP
#define MODES_HARDWARE_LAMP_TYPE_HPP

#include <cassert>

#include "src/compile.h"
#include "src/user/constants.h"
#include "src/system/utils/curves.h"
#include "src/system/utils/constants.h"
#include "src/system/utils/brightness_handle.h"

#include "src/system/ext/math8.h"

#ifdef LMBD_LAMP_TYPE__INDEXABLE
#include "src/system/utils/strip.h"
#include "src/system/physical/output_power.h"
#endif

#include "src/system/platform/time.h"
#include "src/system/physical/sound.h"

#include "src/modes/include/colors/utils.hpp"
#include "src/modes/include/compile.hpp"

#include "src/system/platform/time.h"

/// Provide interface to the physical hardware and other facilities
namespace modes::hardware {

/** \brief Enumeration to write code working with all lamp types
 *
 * See LampTy::flavor and its usage
 */
enum class LampTypes : uint8_t
{
  simple,    ///< Equivalent to LMBD_LAMP_TYPE__SIMPLE
  cct,       ///< Equivalent to LMBD_LAMP_TYPE__CCT
  indexable, ///< Equivalent to LMBD_LAMP_TYPE__INDEXABLE
};

#ifdef LMBD_LAMP_TYPE__SIMPLE
static constexpr LampTypes _lampType = LampTypes::simple;
#endif

#ifdef LMBD_LAMP_TYPE__CCT
static constexpr LampTypes _lampType = LampTypes::cct;
#endif

#ifdef LMBD_LAMP_TYPE__INDEXABLE
static constexpr LampTypes _lampType = LampTypes::indexable;
#endif

/// Currently implemented lamp type, defined at compile-time by the user
static constexpr LampTypes lampType = _lampType;

/** \brief Main interface between the user and the hardware of the lamp
 *
 * TODO: write recipes for best practices when doing Simple/CCT/Indexable
 */
struct LampTy
{
  //
  // state managed by manager
  //

  struct LampConfig
  {
    uint16_t skipFirstLedsForEffect = 0;
    uint16_t skipFirstLedsForAmount = 0;
  };

  LampConfig config;

  //
  // private constructors
  //

  LampTy() = delete;                         ///< \private
  LampTy(const LampTy&) = delete;            ///< \private
  LampTy& operator=(const LampTy&) = delete; ///< \private

#ifdef LMBD_LAMP_TYPE__INDEXABLE
private:
  static constexpr float _fwidth = stripXCoordinates;    ///< \private
  static constexpr float _fheight = stripYCoordinates;   ///< \private
  static constexpr uint16_t _width = floor(_fwidth);     ///< \private
  static constexpr uint16_t _height = floor(_fheight);   ///< \private
  static constexpr uint16_t _ledCount = LED_COUNT;       ///< \private
  LedStrip& strip;                                       ///< \private

public:
  /// \private Constructor used to wrap strip if needed
  LMBD_INLINE LampTy(LedStrip& strip) : config {}, strip {strip}, tick {0}, raw_frame_count {0} {}
#else
private:
  // (placeholder values to avoid bad fails on misuse)
  static constexpr float _fwidth = 16.0;     ///< \private
  static constexpr float _fheight = 16.0;    ///< \private
  static constexpr uint16_t _width = 16;     ///< \private
  static constexpr uint16_t _height = 16;    ///< \private
  static constexpr uint16_t _ledCount = 512; ///< \private

  struct LedStrip ///< \private
  {
  };
  LedStrip fakeStrip; ///< \private
  LedStrip& strip;    ///< \private

public:
  /// \private Constructor used to wrap strip if needed
  LMBD_INLINE LampTy() : config {}, fakeStrip {}, strip {fakeStrip}, tick {0}, raw_frame_count {0} {}
#endif

  //
  // private API
  //

  /// \private (refresh internal const tick variable)
  void LMBD_INLINE refresh_tick_value()
  {
    uint32_t* writable_tick = const_cast<uint32_t*>(&tick);
    *writable_tick = time_ms() / frameDurationMs;

    uint32_t* writable_frame_count = const_cast<uint32_t*>(&raw_frame_count);
    *writable_frame_count += 1; // monotonous
  }

  /** \private Startup sequence of the lamp from a powered-off state
   *
   * In order:
   *  - call lamp.begin()
   *  - call lamp.clear()
   *  - call lamp.show()
   *  - call lamp.setBrightness(get_brightness(), true, true)
   *
   * Supports all flavors, even if most steps are no-op if not indexable
   */
  void LMBD_INLINE startup()
  {
    begin();
    clear();
    show_now();
    setBrightness(brightness::get_brightness(), true, true);
  }

  /** \private Does necessary work to setup lamp from a powered-off state
   *
   * If flavor is LampTypes::indexable then:
   *  - call the "begin" method of the LED strip to let it setup itself
   *
   * Or else, subject to change as other flavors are integrated:
   *  - does nothing
   */
  void LMBD_INLINE begin()
  {
    if constexpr (flavor == LampTypes::indexable)
    {
      strip.begin();
    }
    else
    {
      // (this is optimized out, and is essentially no-op for other flavors)
      assert(true);
    }
  }

  /** \private Does necessary work to display not-yet-shown lamp changes
   *
   * If LampTy::flavor is LampTypes::indexable then:
   *  - update the output of the lamp with currently buffered changes
   *
   * Or else, subject to change as other flavors are integrated:
   *  - does nothing
   */
  void LMBD_INLINE show()
  {
    if constexpr (flavor == LampTypes::indexable)
    {
      strip.show();
    }
    else
    {
      // (this is optimized out, and is essentially no-op for other flavors)
      assert(true);
    }
  }

  /** \private Does necessary work to display not-yet-shown lamp changes
   *
   * If LampTy::flavor is LampTypes::indexable then:
   *  - update the output of the lamp with currently buffered changes IMMEDIATLY
   *
   * Or else, subject to change as other flavors are integrated:
   *  - does nothing
   */
  void LMBD_INLINE show_now()
  {
    if constexpr (flavor == LampTypes::indexable)
    {
      strip.show_now();
    }
    else
    {
      // (this is optimized out, and is essentially no-op for other flavors)
      assert(true);
    }
  }

  //
  // public constants
  //

  /// Which lamp flavor is currently used by the implementation?
  static constexpr LampTypes flavor = lampType;

  /// What is the maximal brightness for that lamp?
  static constexpr brightness_t maxBrightness = ::maxBrightness;

  /// Hardward try to call .loop() every frameDurationMs (12ms for 83.3fps)
  static constexpr uint32_t frameDurationMs = MAIN_LOOP_UPDATE_PERIOD_MS;

  // (we have 12ms and 83.3fps values to be updated in this file)
  static_assert(frameDurationMs == 12, "Update the documentation of .tick :)");

  /** \brief (indexable) Count of indexable LEDs on the lamp
   *
   * Equal to \p LED_COUNT if LampTypes::indexable or else 512
   */
  static constexpr uint16_t ledCount = _ledCount;

  /** \brief (indexable) Width of "pixel space" w/ lamp taken as a LED matrix
   *
   * Equal to \p stripXCoordinates (floor) if LampTypes::indexable or else 16
   */
  static constexpr uint16_t maxWidth = _width;

  /// Width as a precise floating point number, equal to \p stripXCoordinates
  static constexpr float maxWidthFloat = _fwidth;

  /** \brief (indexable) Height of "pixel space" w/ lamp taken as a LED matrix
   *
   * Equal to \p stripYCoordinates (floor) if LampTypes::indexable or else 16
   *
   * Note that the last (incomplete) line of LEDs may not be accounted here.
   */
  static constexpr uint16_t maxHeight = _height;

  /// Height as a precise floating point number, equal to \p stripYCoordinates
  static constexpr float maxHeightFloat = _fheight;

  /// Visually (X,Y) coordinates may appear shifted every \p shiftResidue rows
  static constexpr uint16_t shiftResidue = 1 / (2 * _fwidth - 2 * floor(_fwidth) - 1);

  //
  // public helpers
  //

  /** \brief (indexable) Return a led count in 0-ledCount scaled by \p scale
   *
   * Supports:
   *    - from float (0-1) to count (0-ledCount)
   *    - from uint8_t (0-255) to count (0-ledCount)
   */
  template<typename T> static constexpr LMBD_INLINE uint16_t toLedCount(T scale)
  {
    if constexpr (std::is_same_v<T, float>)
    {
      assert(scale <= 1.0 && "scale is 0-1");
      return ledCount * scale;
    }
    else if constexpr (std::is_same_v<T, uint8_t>)
    {
      uint32_t scaled = ledCount * scale;
      return scaled / 256;
    }
    else
    {
      static_assert(std::is_same_v<T, std::false_type>, "Invalid type, either float or uint8_t required!");
      return 0;
    }
  }

  //
  // public API
  //

  /** \brief Clear lamp to a clean state
   *
   * If LampTy::flavor is LampTypes::indexable then:
   *  - clear LED strip with black pixels
   *
   * Or else, for interoperability:
   *  - does nothing
   */
  void LMBD_INLINE clear()
  {
    if constexpr (flavor == LampTypes::indexable)
    {
      strip.clear();
    }
    else
    {
      // (this is optimized out, and is essentially no-op for other flavors)
      assert(true);
    }
  }

  /** \brief Set brightness of the lamp
   *
   * Several behaviors:
   *  - by default, do not call user callbacks to avoid re-entry
   *  - by default, do call global brightness update handler (from brightness_handle.h)
   *  - also set the brightness in underlying strip object, if necessary
   *
   * Thus:
   *  - if \p skipCallbacks is false, do call user callbacks w/ re-entry risks
   *  - if \p skipUpdateBrightness is true, only set strip object brightness,
   *    or do nothing if LampTy::flavor is not LampTypes::indexable
   *
   * Note that \p skipUpdateBrightness implies \p skipCallbacks implicitly
   */
  void LMBD_INLINE setBrightness(const brightness_t brightness,
                                 const bool skipCallbacks = true,
                                 const bool skipUpdateBrightness = false)
  {
    assert((skipCallbacks || !skipUpdateBrightness) && "implicit callback skip!");

    if constexpr (flavor == LampTypes::indexable)
    {
      constexpr uint8_t minBrightness = 5;
      using curve_t = curves::LinearCurve<brightness_t, uint8_t>;
      static curve_t brightnessCurve({curve_t::point_t {0, minBrightness}, curve_t::point_t {maxBrightness, 255}});

      strip.setBrightness(brightnessCurve.sample(brightness));
    }

    if (!skipUpdateBrightness)
    {
      brightness::update_brightness(brightness, skipCallbacks);
    }
  }

  /// Get brightness of the lamp
  uint8_t LMBD_INLINE getBrightness()
  {
    if constexpr (flavor == LampTypes::indexable)
    {
      return strip.getBrightness();
    }
    else
    {
      return brightness::get_brightness() / maxBrightness * 255;
    }
  }

  /** \brief Fade currently displayed content by \p fadeBy units
   *
   * If LampTy::flavor is LampTypes::indexable then:
   *  - do not change brightness, only scale color of individual LEDs
   *
   * Or else, for other flavors of lamp:
   *  - reduce brightness by \p fadeBy floored at zero
   */
  void LMBD_INLINE fadeToBlackBy(uint8_t fadeBy)
  {
    if constexpr (flavor == LampTypes::indexable)
    {
      strip.fadeToBlackBy(fadeBy);
    }
    else
    {
      uint8_t brightness = getBrightness();
      brightness = (brightness < fadeBy) ? 0 : brightness - fadeBy;
      setBrightness(brightness);
    }
  }

  /** \brief Fill lamp with target light temperature, if supported
   *
   * If LampTy::flavor is LampTypes::simple then:
   *  - does nothing
   *
   * If LampTy::flavor is LampTypes::cct then:
   *  - scale from one supported color to the the other one
   *
   * If LampTy::flavor is LampTypes::indexable then:
   *  - use modes::colors::fromTemp to fill lamp with color
   */
  void LMBD_INLINE setLightTemp(uint8_t temp)
  {
    if constexpr (flavor == LampTypes::indexable)
    {
      fill(colors::fromTemp(temp));
    }
    else if constexpr (flavor == LampTypes::cct)
    {
      // TODO: implement
      assert(false && "not yet implemented :(");
    }
    else
    {
      // -> intentionally do not trigger as error, for interoperability
      assert(true);
    }
  }

  //
  // public API (indexable-only)
  //

  /// (indexable) Return raw underlying strip object
  auto& LMBD_INLINE getLegacyStrip()
  {
    assert(flavor == LampTypes::indexable && "current lamp is not indexable");
    return strip;
  }

  /** \brief (indexable) Fill lamp with target color
   *
   * See modes::colors::fromRGB() to set \p color
   */
  void LMBD_INLINE fill(uint32_t color)
  {
    if constexpr (flavor == LampTypes::indexable)
    {
      for (uint16_t I = 0; I < ledCount; ++I)
      {
        strip.setPixelColor(I, color);
      }
    }
    else
    {
      assert(false && "unsupported");
    }
  }

  /** \brief (indexable) Fill lamp with target color, from start to end
   *
   * - See toLedCount() to set \p start and \p end
   * - See modes::colors::fromRGB() to set \p color
   */
  void LMBD_INLINE fill(uint32_t color, uint16_t start, uint16_t end)
  {
    if constexpr (flavor == LampTypes::indexable)
    {
      assert(start < ledCount && "invalid start parameter");
      assert(end < ledCount && "invalid end parameter");

      for (uint16_t I = start; I < end; ++I)
      {
        strip.setPixelColor(I, color);
      }
    }
    else
    {
      assert(false && "unsupported");
    }
  }

  /** \brief (indexable) Set the n-th LED color
   *
   * See modes::fromRGB() to set \p color
   */
  void LMBD_INLINE setPixelColor(uint16_t n, uint32_t color)
  {
    if constexpr (flavor == LampTypes::indexable)
    {
      if (config.skipFirstLedsForEffect && n < config.skipFirstLedsForAmount)
      {
        return;
      }

      assert(n < ledCount && "invalid LED index");
      strip.setPixelColor(n, color);
    }
    else
    {
      assert(false && "unsupported");
    }
  }

  /** \brief (indexable) Set the X,Y-th LED color
   *
   * See modes::fromRGB() to set \p color
   */
  void LMBD_INLINE setPixelColorXY(uint16_t X, uint16_t Y, uint32_t color) { setPixelColor(to_strip(X, Y), color); }

  /// \brief (physical) Return current sound level in decibels
  float LMBD_INLINE get_sound_level()
  {
    const float level = microphone::get_sound_level_Db();
    return (level > -70) ? level : -70; // avoid -inf or NaN
  }

  /// \brief (physical) Return relative time as milliseconds
  uint32_t LMBD_INLINE get_time_ms() { return time_ms(); }

  /** \brief (physical) Tick number, ever-increasing every frameDurationMs
   *
   * This value increments 83.3 times per second and is updated once per loop.
   *
   * It is based on the internal board clock, that counter is time-based and
   * best used to build animations, but intermediary values may be skipped.
   */
  volatile const uint32_t tick;

  /** \brief (physical) Raw frame count, incremented at an undefined rate
   *
   * This value increments everytime the loop method is called, and its rate
   * may vary depending on hardware and load. If used to build animations, this
   * may cause stutters or visible slowdowns during loads.
   */
  volatile const uint32_t raw_frame_count;
};

} // namespace modes::hardware

#endif

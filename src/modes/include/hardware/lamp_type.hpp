#ifndef MODES_HARDWARE_LAMP_TYPE_HPP
#define MODES_HARDWARE_LAMP_TYPE_HPP

#include <cstring>

#include "src/compile.h"
#include "src/user/constants.h"

#include "src/system/physical/sound.h"
#include "src/system/physical/output_power.h"

#include "src/system/platform/time.h"

#include "src/system/utils/assert.h"
#include "src/system/utils/print.h"
#include "src/system/utils/curves.h"
#include "src/system/utils/constants.h"
#include "src/system/utils/brightness_handle.h"
#include "src/system/utils/utils.h"

#include "src/system/ext/math8.h"

#ifdef LMBD_LAMP_TYPE__INDEXABLE
#include "src/system/utils/strip.h"
#endif

#include "src/modes/include/compile.hpp"
#include "src/modes/include/colors/utils.hpp"
#include "src/modes/include/hardware/coordinates.hpp"

namespace modes {

struct XYTy
{
  uint16_t x, y;
};

static constexpr uint16_t to_strip(uint16_t, uint16_t);
static constexpr XYTy strip_to_XY(uint16_t n);
} // namespace modes

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

#ifdef LMBD_LAMP_TYPE__INDEXABLE
private:
  static constexpr float _fwidth = stripXCoordinates;    ///< \private
  static constexpr float _fheight = stripYCoordinates;   ///< \private
  static constexpr uint16_t _width = floor(_fwidth);     ///< \private
  static constexpr uint16_t _height = floor(_fheight);   ///< \private
  static constexpr uint16_t _ledCount = LED_COUNT;       ///< \private
  static constexpr uint16_t _nbBuffers = stripNbBuffers; ///< \private
  LedStrip& strip;                                       ///< \private

  /// \private oversized buffer size to account for "overflowing" LEDs
  static constexpr uint16_t _safeBufSize = (_width + 1) * (_height + 1);

  /// Type for a uint32_t buffer of exactly ledCount length
  using BufferTy = std::array<uint32_t, _ledCount>;
  // using BufferTy = std::array<uint32_t, _safeBufSize>;

  LampTy() = delete;                         ///< \private
  LampTy(const LampTy&) = delete;            ///< \private
  LampTy& operator=(const LampTy&) = delete; ///< \private

public:
  /// \private Constructor used to wrap strip if needed
  LMBD_INLINE LampTy(LedStrip& strip) : config {}, strip {strip}, now {0}, tick {0}, raw_frame_count {0} {}
#else
private:
  // (placeholder values to avoid bad fails on misuse)
  static constexpr float _fwidth = 23.5451;  ///< \private
  static constexpr float _fheight = 22.51;   ///< \private
  static constexpr uint16_t _width = 23;     ///< \private
  static constexpr uint16_t _height = 22;    ///< \private
  static constexpr uint16_t _ledCount = 530; ///< \private
  static constexpr uint16_t _nbBuffers = 2;  ///< \private

  /// \private oversized buffer size to account for "overflowing" LEDs
  static constexpr uint16_t _safeBufSize = (_width + 1) * (_height + 1);

  /// Type for a uint32_t buffer of exactly ledCount length
  using BufferTy = std::array<uint32_t, _ledCount>;
  // using BufferTy = std::array<uint32_t, _safeBufSize>;

  LampTy(const LampTy&) = delete;            ///< \private
  LampTy& operator=(const LampTy&) = delete; ///< \private

  struct LedStrip ///< \private
  {
    BufferTy _buffers[_nbBuffers];
    COLOR _colors[_ledCount]; // !! slighly smaller than buffers in _buffers !!

    void begin();
    void show();
    void show_now();
    void signal_display();
    void clear();
    void setBrightness(brightness_t);
    uint8_t getBrightness();
    void fadeToBlackBy(uint8_t);
    void setPixelColor(uint16_t, uint32_t);
    uint32_t getPixelColor(uint16_t);
  };
  LedStrip fakeStrip; ///< \private
  LedStrip& strip;    ///< \private

public:
  /// \private Constructor used to wrap strip if needed
  LMBD_INLINE LampTy() : config {}, fakeStrip {}, strip {fakeStrip}, now {0}, tick {0}, raw_frame_count {0} {}
#endif

  //
  // private API
  //

  /// \private (refresh internal const tick variable)
  void LMBD_INLINE refresh_tick_value()
  {
    uint32_t* writeable_now = const_cast<uint32_t*>(&now);
    *writeable_now = time_ms();

    uint32_t* writable_tick = const_cast<uint32_t*>(&tick);
    *writable_tick = now / frameDurationMs;

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
    // set output voltage for indexable
    if constexpr (flavor == LampTypes::indexable)
      outputPower::write_voltage(stripInputVoltage_mV);

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

  /** \private Signal to underlying strip that things are ready to be displayed
   *
   * If LampTy::flavor is LampTypes::indexable then:
   *  - call the .signal_display() method of the underlying strip object
   *
   * Or else, subject to change as other flavors are integrated:
   *  - does nothing
   */
  void LMBD_INLINE signal_display()
  {
    if constexpr (flavor == LampTypes::indexable)
    {
      strip.signal_display();
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

#ifndef LMBD_LAMP_TYPE__INDEXABLE_IS_HD
  // (we have 12ms and 83.3fps values to be updated in this file)
  static_assert(frameDurationMs == 12, "Update the documentation of .tick :)");
#endif

  /// Limit mode-requested brightness changes to one every few frames
  static constexpr uint32_t brightnessDurationMs = frameDurationMs * 2;

  /** \brief (indexable) Count of indexable LEDs on the lamp
   *
   * Equal to \p LED_COUNT if LampTypes::indexable or else 512
   */
  static constexpr uint16_t ledCount = _ledCount;

  /** \brief (indexable) Width of "pixel space" w/ lamp taken as a LED matrix
   *
   * This width equals the smallest maximal X coordinate for which all rows
   * are fully displayed on the lamp taken as an "XY-display" LED matrix.
   *
   * All rows are of length of at least \p maxWidth but some will be of length
   * \p maxOverflowWidth depending on the exact diameter of the lamp and
   * density of the LED strip.
   *
   * Equal to \p stripXCoordinates (floor) if LampTypes::indexable or else 23
   */
  static constexpr uint16_t maxWidth = _width;

  /// Width as a precise floating point number, equal to \p stripXCoordinates
  static constexpr float maxWidthFloat = _fwidth;

  /// Larger width, taken as the absolute maximum X coordinate on all rows
  static constexpr uint16_t maxOverflowWidth = maxWidth + 1;

  /** \brief (indexable) Height of "pixel space" w/ lamp taken as a LED matrix
   *
   * This height equals the largest Y coordinate for which a full \p maxWidth
   * row can be displayed, the last row at coordinate \p maxOverflowHeight
   * being likely to be truncated depending on the exact diameter of the lamp
   * and density of the LED strip.
   *
   * The largest LED matrix fully displayed on the lamp taken as an XY screen
   * display is thus of width \p maxWidth and height \p maxHeight leaving out
   * some "outside" of the default display size.
   *
   * Equal to \p stripYCoordinates (floor) if LampTypes::indexable or else 22
   */
  static constexpr uint16_t maxHeight = _height;

  /// Height as a precise floating point number, equal to \p stripYCoordinates
  static constexpr float maxHeightFloat = _fheight;

  /// Larger height, taken as the absolute maximum Y coordinate, overflowing
  static constexpr uint16_t maxOverflowHeight = maxHeight + 1;

  /// \private Exact visual (X,Y) shift for each row of LED rolled around lamp
  static constexpr float shiftPerTurn = _fwidth - floor(_fwidth - 0.0001);

  /// \private Fixed "period" of the (X,Y) shift for each row (set at 2)
  static constexpr uint16_t shiftPeriod = 2;

  // (while shifting every shiftPeriod, we have a residue accumulating)
  static constexpr float _invShiftResidue = shiftPeriod * (_fwidth - floor(_fwidth - 0.0001));

  /// \private Approximate (broken) visual coordinate X-shift every \p shiftResidue rows
  static constexpr uint16_t shiftResidue = 100.0 / abs(100.0 * _invShiftResidue - 100.0);

  /// \private All **exact** "residue" / amount of X-shift given the Y coordinate
  static constexpr auto allResiduesY = details::computeResidues<maxOverflowHeight>(_fwidth);

  /// \private Is row at Y coordinate slightly longer than \p maxWidth or not?
  static constexpr auto allDeltaResiduesY = details::computeDeltaResidues<maxOverflowHeight>(_fwidth);

  /// \private Return **total** exact accumulated X-shift residue, accross all previous rows
  static constexpr auto extraShiftResiduesY = details::computeExtraShiftResidues<maxOverflowHeight>(_fwidth);

  /// \private Last (final) total X-shift at the end, all row shifts together
  static constexpr int extraShiftTotal = extraShiftResiduesY[maxOverflowHeight - 1];

  /** \brief (indexable) Number of color buffers available for direct access
   *
   * Equal to \p stripNbBuffers if LampTypes::indexable or else 2
   */
  static constexpr uint8_t nbBuffers = _nbBuffers;

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

  /** \brief blip the power gate for set time
   *
   * If LampTy::flavor is LampTypes::indexable then:
   *  - clear LED strip with black pixels
   * Else
   *  - disable output power for a duration
   */
  void LMBD_INLINE blip(const uint32_t duration)
  {
    if constexpr (flavor == LampTypes::indexable)
    {
      // disable strip temporarily
      // should be reactivated by the manager
      tempBrightness(0);
    }
    else
    {
      outputPower::blip(duration);
    }
  }

  /** \brief Set brightness of the lamp
   *
   * Several behaviors:
   *  - by default, do not call user callbacks to avoid re-entry
   *  - by default, do call global brightness update handler (see brightness_handle.h)
   *  - by default, do NOT save brightness using "previous_brightness_update"
   *  - also set the brightness in underlying strip object, if necessary
   *
   * Thus:
   *  - if \p skipCallbacks is false, do call user callbacks w/ re-entry risks
   *  - if \p skipUpdateBrightness is true, only set strip object brightness,
   *    or do nothing if LampTy::flavor is not LampTypes::indexable
   *  - if \p updatePreviousBrightess is true, save brightness and next time
   *    user navigate brightness, use it as starting point (see tempBrightness)
   *
   * Note that calls to `brightness::update_brightness` are throttled to once
   * in every 50ms to avoid freezing user brightness navigation.
   *
   * Note that \p skipUpdateBrightness implies \p skipCallbacks implicitly
   */
  void LMBD_INLINE setBrightness(const brightness_t brightness,
                                 const bool skipCallbacks = true,
                                 const bool skipUpdateBrightness = false,
                                 const bool updatePreviousBrightess = false)
  {
    assert((skipCallbacks || !skipUpdateBrightness) && "implicit callback skip!");

    if constexpr (flavor == LampTypes::indexable)
    {
      constexpr uint8_t minBrightness = 5;
      using curve_t = curves::LinearCurve<brightness_t, uint8_t>;
      static curve_t brightnessCurve({curve_t::point_t {0, minBrightness}, curve_t::point_t {maxBrightness, 255}});

      strip.setBrightness(brightnessCurve.sample(brightness));
    }

    if constexpr (flavor == LampTypes::simple)
    {
      const brightness_t constraintBrightness = min(brightness, maxBrightness);
      if (constraintBrightness >= maxBrightness)
        outputPower::blip(50); // blip

      constexpr uint16_t maxOutputVoltage_mV = stripInputVoltage_mV;
      using curve_t = curves::ExponentialCurve<brightness_t, uint16_t>;
      static curve_t brightnessCurve(
              curve_t::point_t {0, 9400}, curve_t::point_t {maxBrightness, maxOutputVoltage_mV}, 1.0);

      outputPower::write_voltage(round(brightnessCurve.sample(constraintBrightness)));
    }

    if (!skipUpdateBrightness)
    {
      uint32_t last = brightness::when_last_update_brightness();
      if ((this->now - last) > brightnessDurationMs)
        brightness::update_brightness(brightness, skipCallbacks);
    }
    if (updatePreviousBrightess)
      brightness::update_saved_brightness();
  }

  /* \brief Temporarily set brightness, without affecting brightness navigation
   *
   * Skip callbacks.
   *
   * Set brightness, without affecting internal brightness navigation state,
   * which imply that next time an user increases / lowers brightness, it
   * starts again from another internal "previously saved" brightness.
   *
   * Equivalent to ``setBrightness(true, false, false)``
   */
  void LMBD_INLINE tempBrightness(const brightness_t brightness)
  {
    brightness_t current = getBrightness();
    if (current != brightness)
      setBrightness(brightness, true, false, false);
  }

  /* \brief Jump to brightness, affecting the next brightness navigation
   *
   * Skip callbacks.
   *
   * Set brightness, affecting internal brightness navigation state,
   * which imply that next time an user increases / lowers brightness, it
   * starts again from the provided brightness.
   *
   * Equivalent to ``setBrightness(true, false, true)``
   */
  void LMBD_INLINE jumpBrightness(const brightness_t brightness) { setBrightness(brightness, true, false, true); }

  /* \brief Get brightness of the lamp
   *
   * If \p readPreviousBrightness is true, return the internally saved
   * brightness state, used for user navigation of brightness.
   *
   * TODO: determine if we should keep 0-255 or have minBrightness-maxBrightness
   */
  brightness_t LMBD_INLINE getBrightness(const bool readPreviousBrightness = false)
  {
    if constexpr (flavor == LampTypes::indexable)
    {
      if (readPreviousBrightness)
        return brightness::get_saved_brightness();
      return strip.getBrightness();
    }
    else
    {
      if (readPreviousBrightness)
        return brightness::get_saved_brightness();
      else
        return brightness::get_brightness();
    }
  }

  /// Alias for ``getBrightness(true)``
  brightness_t LMBD_INLINE getSavedBrightness() { return getBrightness(true); }

  /// Reset brightness to internal saved brightness, skip callbacks
  void LMBD_INLINE restoreBrightness()
  {
    brightness_t current = getBrightness();
    brightness_t saved = getBrightness(true);

    if (current != saved)
      setBrightness(saved, true, false, false);
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

  /** \brief Blur currently displayed content by \p blurBy units
   * /!\ Only applied along the strip, not in 2D /!\
   *
   * If LampTy::flavor is LampTypes::indexable then:
   *  - do not change brightness, only scale color of individual LEDs
   *
   * Or else, for other flavors of lamp:
   *  - blue colors by \p blurBy floored at zero
   */
  void LMBD_INLINE blur(uint8_t blurBy)
  {
    if (blurBy == 0)
      return; // optimization: 0 means "don't blur"
    uint8_t keep = 255 - blurBy;
    uint8_t seep = blurBy >> 1;

    uint32_t carryover = 0;
    for (unsigned i = 0; i < ledCount; i++)
    {
      uint32_t cur = strip.getPixelColor(i);
      uint32_t c = cur;
      uint32_t part = colors::fade<false>(c, seep);
      cur = colors::add<true>(colors::fade<false>(c, keep), carryover);
      if (i > 0)
      {
        c = strip.getPixelColor(i - 1);
        strip.setPixelColor(i - 1, colors::add<true>(c, part));
      }
      strip.setPixelColor(i, cur);
      carryover = part;
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

  /** \brief (indexable) Fill lamp with target color
   *
   * See modes::colors::fromRGB() to set \p color
   */
  void LMBD_INLINE fill(uint32_t color) { fill(color, 0, ledCount); }

  /** \brief (indexable) Fill lamp with target palette, from start to end
   *
   * - See toLedCount() to set \p start and \p end
   * - See modes::colors::PaletteTy to set \p palette
   */
  void LMBD_INLINE fill(const colors::PaletteTy& palette, uint16_t start, uint16_t end)
  {
    if constexpr (flavor == LampTypes::indexable)
    {
      assert(start < ledCount && "invalid start parameter");
      assert(end < ledCount && "invalid end parameter");

      for (uint16_t I = start; I < end; ++I)
      {
        // map index to [0; 255]
        const auto color = colors::from_palette<false, uint8_t>(I / static_cast<float>(ledCount) * 255, palette);
        strip.setPixelColor(I, color);
      }
    }
    else
    {
      assert(false && "unsupported");
    }
  }

  /** \brief (indexable) Fill lamp with target palette
   * - See modes::colors::PaletteTy to set \p palette
   */
  void LMBD_INLINE fill(const colors::PaletteTy& palette) { fill(palette, 0, ledCount); }

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

  /** \brief (indexable) Get the n-th LED color
   */
  uint32_t LMBD_INLINE getPixelColor(uint16_t n)
  {
    if constexpr (flavor == LampTypes::indexable)
    {
      if (config.skipFirstLedsForEffect && n < config.skipFirstLedsForAmount)
      {
        return 0;
      }

      assert(n < ledCount && "invalid LED index");
      return strip.getPixelColor(n);
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

  /** \brief (indexable) Given X,Y coordinate, return a corresponding LED index
   *
   * Uses modes::to_strip as implementation (both function are equivalent)
   */
  static uint16_t LMBD_INLINE fromXYtoStripIndex(uint16_t X, uint16_t Y) { return to_strip(X, Y); }

  /** \brief (indexable) Given a LED index, return a corresponding X,Y value
   *
   * Uses modes::strip_to_XY as implementation (both function are equivalent)
   */
  static XYTy LMBD_INLINE fromStripIndexToXY(uint16_t N) { return strip_to_XY(N); }

  /** \brief Get a reference to the \p bufIdx temporary buffer
   *
   * These buffers can be used for computations by the active mode, but their
   * content may be erased if the mode is reset or if used elsewhere in code.
   */
  template<uint8_t bufIdx = 0> BufferTy& LMBD_INLINE getTempBuffer()
  {
    static_assert(bufIdx < nbBuffers, "bufIdx must be lower than nbBuffers");
    BufferTy& buffer = strip._buffers[bufIdx];
    return buffer;
  }

  /** \brief Fill temporary buffer \p bufIdx with given \p value
   *
   * Equivalent to ``getTempBuffer().fill(value)``
   */
  template<uint8_t bufIdx = 0, typename T> void LMBD_INLINE fillTempBuffer(const T& value)
  {
    getTempBuffer<bufIdx>().fill(value);
  }

  /** \brief (indexable) Display \p bufIdx temporary buffer as LED colors
   *
   * This ``memcpy`` the selected buffer to the internal strip color buffer.
   */
  template<uint8_t bufIdx = 0> void setColorsFromBuffer()
  {
    static_assert(sizeof(BufferTy) == sizeof(strip._colors));
    const BufferTy& buffer = getTempBuffer<bufIdx>();
    if (config.skipFirstLedsForEffect)
    {
      uint16_t Idx = config.skipFirstLedsForAmount;
      uint16_t Sz = sizeof(strip._colors) - Idx * sizeof(uint32_t);
      memcpy(&strip._colors[Idx], &buffer[Idx], Sz);
    }
    else
    {
      memcpy(strip._colors, buffer.data(), sizeof(strip._colors));
    }
  }

  template<uint8_t dstBufIdx = 0, uint8_t srcBufIdx = 1> void setColorsFromMixedBuffers(float phase)
  {
    static_assert(sizeof(BufferTy) == sizeof(strip._colors));
    const BufferTy& dstBuf = getTempBuffer<dstBufIdx>();
    const BufferTy& srcBuf = getTempBuffer<srcBufIdx>();

    uint16_t start = 0, end = srcBuf.size();
    if (config.skipFirstLedsForEffect)
    {
      start = config.skipFirstLedsForAmount;
    }

    for (uint16_t I = start; I < end; ++I)
    {
      COLOR src, dst;
      src.color = srcBuf[I];
      dst.color = dstBuf[I];
      strip._colors[I].color = utils::get_gradient(src.color, dst.color, phase);
    }
  }

  /** \brief (indexable) Display \p bufIdx temporary buffer, but reversed
   */
  template<uint8_t bufIdx = 0> void setColorsFromBufferReversed(bool skipLastLine)
  {
    static_assert(sizeof(BufferTy) == sizeof(strip._colors));
    const BufferTy& buffer = getTempBuffer<bufIdx>();

    uint16_t start = 0, end = buffer.size();
    if (config.skipFirstLedsForEffect)
    {
      start = config.skipFirstLedsForAmount;
    }

    if (skipLastLine)
    {
      end = maxWidth * maxHeight;
    }

    for (uint16_t I = 0, J = start; I < end - start; ++I, ++J)
    {
      strip._colors[J].color = buffer[end - start - I - 1];
    }
  }

  /** \brief (indexable) Copy all current LEDs color to \p bufIdx temp. buffer
   *
   * If \p forceFullRead is True, also copy skipped content.
   */
  template<uint8_t bufIdx = 0, bool forceFullRead = false> void getColorsToBuffer()
  {
    static_assert(sizeof(BufferTy) == sizeof(strip._colors));

    BufferTy& buffer = getTempBuffer<bufIdx>();
    if (!forceFullRead && config.skipFirstLedsForEffect)
    {
      uint16_t Idx = config.skipFirstLedsForAmount;
      uint16_t Sz = sizeof(strip._colors) - Idx * sizeof(uint32_t);

      buffer.fill(0);
      memcpy(&buffer[Idx], &strip._colors[Idx], Sz);
    }
    else
    {
      memcpy(buffer.data(), strip._colors, sizeof(strip._colors));
    }
  }

  /// \brief (physical) Return current sound level in decibels
  float LMBD_INLINE get_sound_level()
  {
    const float level = microphone::get_sound_level_Db();
    return (level > -70) ? level : -70; // avoid -inf or NaN
  }

  /** \brief (physical) The "now" on milliseconds, updated just before loop.
   *
   * This value is \p get_time_ms() called once and used as basis for \p tick
   * and may wrap around every three weeks of functioning.
   */
  volatile const uint32_t now;

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

namespace modes {

/// \brief Convert \p x and \p y coordinates to a linear position on strip
static constexpr uint16_t to_strip(uint16_t x, uint16_t y)
{
  // maxHeight is the last "full" row and maxOverflowHeight is truncated row
  //  -> user max use to_strip(x, maxOverflowHeight) to set truncated row
  if (y >= hardware::LampTy::maxOverflowHeight)
  {
    y = hardware::LampTy::maxOverflowHeight - 1;
  }

  // if row is longer (because next row gets a shift) use maxOverflowWidth
  if (hardware::LampTy::allDeltaResiduesY[y])
  {
    if (x >= hardware::LampTy::maxOverflowWidth)
    {
      x = hardware::LampTy::maxOverflowWidth - 1;
    }

    // if row is aligned to XY maxWidth/maxHeight matrix, use maxWidth instead
  }
  else if (x >= hardware::LampTy::maxWidth)
  {
    x = hardware::LampTy::maxWidth - 1;
  }

  // final result is capped by the "real" LED count of the strip
  uint16_t n = x + y * hardware::LampTy::maxWidth + hardware::LampTy::allResiduesY[y];
  if (n >= hardware::LampTy::ledCount)
    n = hardware::LampTy::ledCount - 1;
  return n;
}

/// \brief Convert a \p n index on the LED strip to a matching XY coordinate
static constexpr XYTy strip_to_XY(uint16_t n)
{
  // (saturates N to ledCount)
  if (n >= hardware::LampTy::ledCount)
    n = hardware::LampTy::ledCount - 1;

  uint16_t y = 0;
  while (y * hardware::LampTy::maxWidth + hardware::LampTy::allResiduesY[y] <= n)
    y += 1;

  if (y == 0)
    return {0, 0};
  y -= 1;

  uint16_t x = n - y * hardware::LampTy::maxWidth - hardware::LampTy::allResiduesY[y];
  return {x, y};
}

} // namespace modes

#endif

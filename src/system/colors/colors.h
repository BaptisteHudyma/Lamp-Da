#ifndef COLORS_H
#define COLORS_H

// this file is active only if LMBD_LAMP_TYPE=indexable
#ifdef LMBD_LAMP_TYPE__INDEXABLE

#include <cstdint>

#include "src/system/colors/palettes.h"
#include "src/system/utils/constants.h"
#include "src/system/utils/utils.h"

#include "src/system/platform/time.h"

// min color update frequency
constexpr uint32_t COLOR_TIMING_UPDATE = MAIN_LOOP_UPDATE_PERIOD_MS * 3;

/**
 * \brief Basic color generation class
 */
class Color
{
public:
  uint32_t get_color(const uint16_t index = 0, const uint16_t maxIndex = 0) const
  {
    // prevent the user to break the system
    return get_color_internal(lmpd_constrain<uint16_t>(index, 0, maxIndex), maxIndex);
  }

  /**
   * \brief reset the color
   */
  virtual void reset() = 0;

private:
  virtual uint32_t get_color_internal(const uint16_t index = 0, const uint16_t maxIndex = 0) const = 0;
};

/**
 * \brief Basic dynamic color generation class
 */
class DynamicColor : public Color
{
public:
  /**
   * \brief check for animation update
   * \return true if the animation was updated
   */
  virtual bool update()
  {
    const long unsigned currentMillis = time_ms();
    if (currentMillis - _lastUpdate > COLOR_TIMING_UPDATE) // ms update period
    {
      internal_update(currentMillis - _lastUpdate);
      _lastUpdate = currentMillis;
      return true;
    }
    return false;
  }

protected:
  virtual void internal_update(const uint32_t deltaTimeMilli) = 0;
  long unsigned _lastUpdate; // time of the last update, in ms
};

class IndexedColor : public Color
{
public:
  virtual void update(const uint8_t index) = 0;
};

/**
 * \brief Generate a solid color
 */
class GenerateSolidColor : public Color
{
public:
  GenerateSolidColor(const uint32_t color) : _color(color) {};

  uint32_t get_color_internal(const uint16_t index = 0, const uint16_t maxIndex = 0) const override;

  void reset() override {};

private:
  uint32_t _color;
};

/**
 * \brief Generate a rainbow color from top to bottom
 */
class GenerateRainbowColor : public Color
{
public:
  GenerateRainbowColor() {};

  uint32_t get_color_internal(const uint16_t index, const uint16_t maxIndex) const override;

  void reset() override {};
};

/**
 * \brief Generate a gradient color from top to bottom
 */
class GenerateGradientColor : public Color
{
public:
  GenerateGradientColor(const uint32_t colorStart, const uint32_t colorEnd) :
    _colorStart(colorStart),
    _colorEnd(colorEnd) {};

  uint32_t get_color_internal(const uint16_t index, const uint16_t maxIndex) const override;

  void reset() override {};

private:
  uint32_t _colorStart;
  uint32_t _colorEnd;
};

/**
 * \brief Generate a gradient color from top to bottom
 */
class GenerateRoundColor : public Color
{
public:
  GenerateRoundColor() {};

  uint32_t get_color_internal(const uint16_t index, const uint16_t maxIndex) const override;

  void reset() override {};
};

/**
 * \brief Generate a rainbow color that loop around the display
 * \param[in] period The period of the animation
 */
class GenerateRainbowSwirl : public DynamicColor
{
public:
  GenerateRainbowSwirl(const uint32_t period) :
    _increment(UINT16_MAX / (period / COLOR_TIMING_UPDATE)),
    _firstPixelHue(0)
  {
  }

  uint32_t get_color_internal(const uint16_t index, const uint16_t maxIndex) const override;

  void reset() override { _firstPixelHue = 0; };

private:
  /**
   * \brief Call when you want the animation to progress
   */
  void internal_update(const uint32_t deltaTimeMilli) override { _firstPixelHue += _increment; };

  uint32_t _increment;
  uint16_t _firstPixelHue;
};

/**
 * \brief Display color from a palette
 * \param[in] palette The palette to use
 */
class GeneratePalette : public DynamicColor
{
public:
  GeneratePalette(const uint8_t increment, const palette_t& palette) :
    _increment(increment),
    _index(0),
    _paletteRef(&palette)
  {
  }

  uint32_t get_color_internal(const uint16_t index, const uint16_t maxIndex) const override;

  void reset() override { _index = 0; };

private:
  /**
   * \brief Call when you want the animation to progress
   */
  void internal_update(const uint32_t deltaTimeMilli) override { _index += _increment; };

  uint8_t _increment;
  uint16_t _index;
  const palette_t* _paletteRef;
};

/**
 * \brief Step through a palette
 * \param[in] palette The palette to use
 */
class GeneratePaletteStep : public DynamicColor
{
public:
  GeneratePaletteStep(const palette_t& palette) : _index(0), _paletteRef(&palette) {}

  uint32_t get_color_internal(const uint16_t index, const uint16_t maxIndex) const override;

  void reset() override { _index = 0; };

private:
  /**
   * \brief Call when you want the animation to progress
   */
  void internal_update(const uint32_t deltaTimeMilli) override { _index += 1; };

  uint8_t _index;
  const palette_t* _paletteRef;
};

class GeneratePaletteIndexed : public IndexedColor
{
public:
  GeneratePaletteIndexed(const palette_t& palette) : _index(0), _paletteRef(&palette) {}

  uint32_t get_color_internal(const uint16_t index, const uint16_t maxIndex) const override;

  void update(const uint8_t index) override { _index = index; }

  void reset() override { _index = 0; };

private:
  uint8_t _index;
  const palette_t* _paletteRef;
};

/**
 * \brief Generate a rainbow color that loop around the display
 * \param[in] period The period of the animation
 */
class GenerateRainbowPulse : public DynamicColor
{
public:
  GenerateRainbowPulse(const uint8_t colorDivisions) : _currentPixelHue(0)
  {
    _increment = max<float>(UINT16_MAX / static_cast<float>(colorDivisions), 1.0f);
  }

  uint32_t get_color_internal(const uint16_t index, const uint16_t maxIndex) const override;

  void reset() override { _currentPixelHue = 0; };

private:
  /**
   * \brief Call when you want the animation to progress
   */
  void internal_update(const uint32_t deltaTimeMilli) override { _currentPixelHue += _increment; };

  uint16_t _increment;
  uint16_t _currentPixelHue;
};

class GenerateRainbowIndex : public IndexedColor
{
public:
  GenerateRainbowIndex(const uint8_t colorDivisions) :
    _increment(UINT16_MAX / static_cast<float>(colorDivisions)),
    _currentPixelHue(0)
  {
  }

  uint32_t get_color_internal(const uint16_t index, const uint16_t maxIndex) const override;

  void update(const uint8_t index) override { _currentPixelHue = float(index) * _increment; }

  void reset() override { _currentPixelHue = 0; };

private:
  float _increment;
  uint16_t _currentPixelHue;
};

// generate a rainbow with pastel colors
class GeneratePastelPulse : public DynamicColor
{
public:
  GeneratePastelPulse(const uint8_t colorDivisions) : _currentPixelHue(0)
  {
    _increment = max<float>(UINT16_MAX / static_cast<float>(colorDivisions), 1.0f);
  }

  uint32_t get_color_internal(const uint16_t index, const uint16_t maxIndex) const override;

  void reset() override { _currentPixelHue = 0; };

private:
  /**
   * \brief Call when you want the animation to progress
   */
  void internal_update(const uint32_t deltaTimeMilli) override { _currentPixelHue += _increment; };

  uint16_t _increment;
  uint16_t _currentPixelHue;
};

/**
 * \brief Get a random color. Color changes at each update() call
 */
class GenerateRandomColor : public DynamicColor
{
public:
  GenerateRandomColor();

  uint32_t get_color_internal(const uint16_t index, const uint16_t maxIndex) const override { return _color; }

  void reset() override {};

private:
  /**
   * \brief Call when you want the animation to progress
   */
  void internal_update(const uint32_t deltaTimeMilli) override;

  uint32_t _color;
};

/**
 * \brief Get a color. Color changes to it's complement at each update() call
 */
class GenerateComplementaryColor : public DynamicColor
{
public:
  GenerateComplementaryColor(const float randomVariation) : _color(0), _randomVariation(randomVariation) { reset(); }

  uint32_t get_color_internal(const uint16_t index, const uint16_t maxIndex) const override { return _color; }

  void reset() override { internal_update(0); };

private:
  /**
   * \brief Call when you want the animation to progress
   */
  void internal_update(const uint32_t deltaTimeMilli) override;

  uint32_t _color;
  float _randomVariation;
};

#endif

#endif

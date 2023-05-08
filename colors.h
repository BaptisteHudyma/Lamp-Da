#ifndef COLORS_H
#define COLORS_H

#include <Arduino.h>
#include <cstdint>
#include "constants.h"

/**
 * \brief Basic color generation class
 */
class Color
{
    public:
    virtual uint32_t get_color(const uint16_t index = 0, const uint16_t maxIndex = 0) const = 0;
};

/**
 * \brief Basic dynamic color generation class
 */
const uint8_t updatePeriod = 30;   // milliseconds
class DynamicColor : public Color
{
    public:
    /**
     * \brief check for animation update
     * \return true if the animation was updated
     */
    virtual bool update()
    {
        const long unsigned currentMillis = millis();
        if (currentMillis - _lastUpdate > 30)   // 30 ms update period
        {
            internal_update(currentMillis - _lastUpdate);
            _lastUpdate = currentMillis;
            return true;
        }
        return false;
    }

    /**
     * \brief reset the animation
     */
    virtual void reset() = 0;

    protected:
    virtual void internal_update(const uint32_t deltaTimeMilli) = 0;
    long unsigned _lastUpdate;  // time of the last update, in ms
};

/**
 * \brief Generate a solid color
 */
class GenerateSolidColor : public Color
{
    public:
    GenerateSolidColor(const uint32_t color) : _color(color) {};

    uint32_t get_color(const uint16_t index = 0, const uint16_t maxIndex = 0) const override;

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

    uint32_t get_color(const uint16_t index, const uint16_t maxIndex) const override;
};

/**
 * \brief Generate a gradient color from top to bottom
 */
class GenerateGradientColor : public Color
{
    public:
    GenerateGradientColor(const uint32_t colorStart, const uint32_t colorEnd) : _colorStart(colorStart), _colorEnd(colorEnd) {};

    uint32_t get_color(const uint16_t index, const uint16_t maxIndex) const override;

    private:
    uint32_t _colorStart;
    uint32_t _colorEnd;
};

/**
 * \brief Generate a rainbow color that loop around the display
 * \param[in] period The period of the animation
 */
class GenerateRainbowSwirl : public DynamicColor
{
public:
    GenerateRainbowSwirl(const uint32_t period)
    : _increment(UINT16_MAX / (period / updatePeriod)), _firstPixelHue(0)
    {}

    uint32_t get_color(const uint16_t index, const uint16_t maxIndex) const override;
    
    void reset() override { _firstPixelHue = 0; };

private:
    /**
     * \brief Call when you want the animation to progress
     */
    void internal_update(const uint32_t deltaTimeMilli) override {
        _firstPixelHue += _increment;
    };

    uint32_t _increment;
    uint16_t _firstPixelHue;
};


/**
 * \brief Generate a rainbow color that loop around the display
 * \param[in] period The period of the animation
 */
class GenerateRainbowPulse : public DynamicColor
{
public:
    GenerateRainbowPulse(const uint8_t colorDivisions)
    : _increment(UINT16_MAX / colorDivisions), _currentPixelHue(0)
    {}

    uint32_t get_color(const uint16_t index, const uint16_t maxIndex) const override;
    
    void reset() override { _currentPixelHue = 0; };

private:
    /**
     * \brief Call when you want the animation to progress
     */
    void internal_update(const uint32_t deltaTimeMilli) override {
        _currentPixelHue += _increment;
    };

    uint32_t _increment;
    uint16_t _currentPixelHue;
};

#endif
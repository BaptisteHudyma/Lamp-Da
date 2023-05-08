#ifndef COLORS_H
#define COLORS_H

#include <cstdint>

class Color
{
    public:
    virtual uint32_t get_color(const uint16_t index = 0, const uint16_t maxIndex = 0, const uint32_t previousColor = 0) const = 0;
};

/**
 * \brief Generate a solid color
 */
class GenerateSolidColor : public Color
{
    public:
    GenerateSolidColor(const uint32_t color) : _color(color) {};

    uint32_t get_color(const uint16_t index = 0, const uint16_t maxIndex = 0, const uint32_t previousColor = 0) const override;

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

    uint32_t get_color(const uint16_t index, const uint16_t maxIndex, const uint32_t previousColor = 0) const override;
};

/**
 * \brief Generate a gradient color from top to bottom
 */
class GenerateGradientColor : public Color
{
    public:
    GenerateGradientColor(const uint32_t colorStart, const uint32_t colorEnd) : _colorStart(colorStart), _colorEnd(colorEnd) {};

    uint32_t get_color(const uint16_t index, const uint16_t maxIndex, const uint32_t previousColor = 0) const override;

    private:
    uint32_t _colorStart;
    uint32_t _colorEnd;
};

#endif
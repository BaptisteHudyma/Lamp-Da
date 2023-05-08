#ifndef COLORS_H
#define COLORS_H

#include <Adafruit_NeoPixel.h>
#include <cstdint>
#include "constants.h"


class Color
{
    public:
    virtual uint32_t get_color(const uint16_t index, const uint16_t maxIndex, const uint32_t previousColor) const = 0;
};

/**
 * \brief Generate a solid color
 */
class GenerateSolidColor : public Color
{
    public:
    GenerateSolidColor(const uint32_t color) : _color(color) {};

    uint32_t get_color(const uint16_t index = 0, const uint16_t maxIndex = 0, const uint32_t previousColor = 0) const override
    {
        return _color;
    }

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

    uint32_t get_color(const uint16_t index = 0, const uint16_t maxIndex = 0, const uint32_t previousColor = 0) const override
    {
        const uint16_t hue = map(index, 0, maxIndex, 0, MAX_UINT16_T);
        return Adafruit_NeoPixel::gamma32(Adafruit_NeoPixel::ColorHSV(hue));  //  Set pixel's color (in RAM)
    }
};

#endif
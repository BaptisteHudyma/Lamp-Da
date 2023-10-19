#ifndef STRIP_H
#define STRIP_H

#include <Adafruit_NeoPixel.h>
#include <cstdint>
#include "constants.h"

/**
 * \brief Use this to convert color to bytes
 */
union COLOR
{
    uint32_t color;
    
    struct
    {
        uint8_t blue;
        uint8_t green;
        uint8_t red;
        uint8_t white;
    };
};

class LedStrip : public Adafruit_NeoPixel
{
    public:
    LedStrip(int16_t pin = 6,
            neoPixelType type = NEO_GRB + NEO_KHZ800) :
        Adafruit_NeoPixel(LED_COUNT, pin, type)
    {
        COLOR c;
        c.color = 0;
        for(uint16_t i = 0; i < LED_COUNT; ++i)
            _colors[i] = c;
    }

    void setPixelColor(uint16_t n, COLOR c)
    {
        _colors[n] = c;
        Adafruit_NeoPixel::setPixelColor(n, c.color);
    }

    void setPixelColor(uint16_t n, uint8_t r, uint8_t g, uint8_t b)
    {
        COLOR c;
        c.red = r;
        c.green = g;
        c.blue = b;

        setPixelColor(n, c);
    }

    void setPixelColor(uint16_t n, uint8_t r, uint8_t g, uint8_t b, uint8_t w)
    {
        COLOR c;
        c.red = r;
        c.green = g;
        c.blue = b;
        c.white = w;

        setPixelColor(n, c);
    }

    void setPixelColor(uint16_t n, uint32_t c)
    {
        COLOR co;
        co.color = c;

        setPixelColor(n, co);
    }

    uint32_t getPixelColor(uint16_t n) const
    {
        return _colors[n].color;
    }

    private:
    COLOR _colors[LED_COUNT];
};

#endif
#ifndef STRIP_H
#define STRIP_H

#include <Adafruit_NeoPixel.h>
#include <cstdint>
#include <cstring>
#include "constants.h"
#include "coordinates.h"

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
        {
            _colors[i] = c;
        
            lampCoordinates[i] = to_lamp(i);
        }
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

    inline Cartesian get_lamp_coordinates(uint16_t n) const
    {
        return lampCoordinates[n];
    }

    uint32_t* get_buffer_ptr(const uint8_t index)
    {
        return _buffers[index];
    }

    void buffer_current_colors(const uint8_t index)
    {
        memcpy(_buffers[index], _colors, sizeof(_colors));
    }

    void fill_buffer(const uint8_t index, const uint32_t value)
    {
        memset(_buffers[index], value, sizeof(_buffers[index]));
    }

    uint8_t _buffer8b[LED_COUNT];
    uint16_t _buffer16b[LED_COUNT];
    
    private:
    COLOR _colors[LED_COUNT];
    // buffers for computations
    uint32_t _buffers[2][LED_COUNT];

    // save the expensive computation on world coordinates
    Cartesian lampCoordinates[LED_COUNT];
};

#endif
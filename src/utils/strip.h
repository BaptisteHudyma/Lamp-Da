#ifndef STRIP_H
#define STRIP_H

#include <Adafruit_NeoPixel.h>
#include <cstdint>
#include <cstring>
#include "constants.h"
#include "coordinates.h"

#include "utils.h"

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

    void show()
    {
        if (hasSomeChanges)
        {
            // only show if some changes were made
            Adafruit_NeoPixel::show();
        }
        hasSomeChanges = false;
    }

    void setPixelColor(uint16_t n, COLOR c)
    {
        _colors[n] = c;
        hasSomeChanges = true;
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

    void setPixelColorXY(uint16_t x, uint16_t y, COLOR c)
    {
        setPixelColor(to_strip(x, y), c);
    }

    void setPixelColorXY(uint16_t x, uint16_t y, uint32_t c)
    {
        setPixelColor(to_strip(x, y), c);
    }

    void fadeToBlackBy(const uint8_t fadeBy)
    {
        if (fadeBy == 0) return;   // optimization - no scaling to apply

        for(uint16_t i = 0; i < LED_COUNT; ++i)
        {
            COLOR c;
            c.color = getPixelColor(i);
            setPixelColor(i, utils::color_fade(c, 255-fadeBy));
        }
    }

    void blur(uint8_t blur_amount)
    {
        if (blur_amount == 0) return; // optimization: 0 means "don't blur"
        uint8_t keep = 255 - blur_amount;
        uint8_t seep = blur_amount >> 1;
        COLOR carryover;
        carryover.color = 0;
        for (unsigned i = 0; i < LED_COUNT; i++) {
            COLOR cur;
            cur.color = getPixelColor(i);
            COLOR c = cur;
            COLOR part = utils::color_fade(c, seep);
            cur = utils::color_add(utils::color_fade(c, keep), carryover, true);
            if (i > 0) {
                c.color = getPixelColor(i-1);
                setPixelColor(i-1, utils::color_add(c, part, true));
            }
            setPixelColor(i, cur);
            carryover = part;
        }
    }

    uint32_t getPixelColor(uint16_t n) const
    {
        return _colors[n].color;
    }

    uint32_t getRawPixelColor(uint16_t n) const
    {
        return Adafruit_NeoPixel::getPixelColor(n);
    }

    void clear()
    {
        COLOR c;
        c.color = 0;
        for(uint16_t i = 0; i < LED_COUNT; ++i)
        {
            _colors[i] = c;
        };
        hasSomeChanges = true;
        Adafruit_NeoPixel::clear();
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

    private:
    bool hasSomeChanges;
};

#endif
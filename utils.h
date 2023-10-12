#ifndef UTILS_H
#define UTILS_H

#include "constants.h"
#include "colors.h"
#include <cstdint>

namespace utils {

#define SQR(x) ((x)*(x))
#define POW2(x) SQR(x)
#define POW3(x) ((x)*(x)*(x))
#define POW4(x) (POW2(x)*POW2(x))
#define POW7(x) (POW3(x)*POW3(x)*(x))
#define DegToRad(x) ((x)*M_PI/180)

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

namespace ColorSpace {

class Base
{
    public:
    virtual COLOR get_rgb() const = 0;
};

class XYZ : public Base {
    public:
    XYZ(const COLOR& c)
    {
        from_rgb(c);
    };
    XYZ(double x, double y, double z) :
    x(x), y(y), z(z) 
    {};

    static XYZ get_white()
    {
        static const XYZ white(95.047, 100.000, 108.883);
        return white;
    }

    COLOR get_rgb() const override;

    void from_rgb(const COLOR& rgb);

    double x;
    double y;
    double z;
};

class RGB : public Base {
    public:
    RGB(uint8_t red, uint8_t green, uint8_t blue)
    {
        _color.red = red;
        _color.green = green;
        _color.blue = blue;
    }

    RGB(const uint32_t color)
    {
        _color.color = color;
    }

    COLOR get_rgb() const override
    {
        return _color;
    }

    private:
    COLOR _color;
};

class HSV : public Base {
    public:
    HSV(const COLOR& c)
    {
        from_rgb(c);
    };
    HSV(double h, double s, double v) :
    h(h), s(s), v(v) 
    {};

    COLOR get_rgb() const override;

    void from_rgb(const COLOR& rgb);

    uint16_t get_scaled_hue() const
    {
        return h / 360.0 * UINT16_MAX;
    }

    double h;
    double s;
    double v;
};

class LAB : public Base
{
    public:
    LAB(const COLOR& c)
    {
        from_rgb(c);
    };
    LAB(const double l, const double a, const double b) : 
    l(l), a(a), b(b)
    {};

    // get the rgb form (for display)
    COLOR get_rgb() const override;

    void from_rgb(const COLOR& rgb);

    double l;
    double a;
    double b;
};

class LCH : public Base
{
    public:
    LCH(const COLOR& c)
    {
        from_rgb(c);
    };
    LCH(const double l, const double c, const double h) : 
    l(l), c(c), h(h)
    {};

    // get the rgb form (for display)
    COLOR get_rgb() const override;

    void from_rgb(const COLOR& rgb);

    uint16_t get_scaled_hue() const
    {
        return h / 360.0 * UINT16_MAX;
    }

    double l;
    double c;
    double h;   // 0 - 360
};

class OKLAB : public Base
{
    public:
    OKLAB(const COLOR& c)
    {
        from_rgb(c);
    };
    OKLAB(const double l, const double a, const double b) : 
    l(l), a(a), b(b)
    {};

    // get the rgb form (for display)
    COLOR get_rgb() const override;

    void from_rgb(const COLOR& rgb);

    double l;
    double a;
    double b;
};

class OKLCH : public Base
{
    public:
    OKLCH(const COLOR& c)
    {
        from_rgb(c);
    };
    OKLCH(const double l, const double c, const double h) : 
    l(l), c(c), h(h)
    {};

    // get the rgb form (for display)
    COLOR get_rgb() const override;

    void from_rgb(const COLOR& rgb);

    uint16_t get_scaled_hue() const
    {
        return h / 360.0 * UINT16_MAX;
    }

    double l;
    double c;
    double h;   // 0 - 360
};

}

/**
 * \brief Compute a random color
 */
uint32_t get_random_color();

/**
 * \brief Compute the complementary color of the given color
 * \param[in] color The color to find a complement for
 * \return the complementary color
 */
uint32_t get_complementary_color(const uint32_t color);

/**
 * \brief Compute the complementary color of the given color, with a random variation
 * \param[in] color The color to find a complement for
 * \param[in] tolerance between 0 and 1, the variation tolerance. 1 will give a totally random color, 0 will return the base complementary color
 * \return the random complementary color
 */
uint32_t get_random_complementary_color(const uint32_t color, const float tolerance);

/**
 * \brief Return the color gradient between colorStart to colorEnd
 * \param[in] colorStart Start color of the gradient
 * \param[in] colorEnd End color of the gradient
 * \param[in] level between 0 and 1, the gradient between the two colors
 */
uint32_t get_gradient(const uint32_t colorStart, const uint32_t colorEnd, const float level);

uint32_t hue2rgbSinus(const uint16_t angle);

};

#endif
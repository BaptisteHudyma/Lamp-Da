#include <Arduino.h>

#define LMBD_LAMP_TYPE__INDEXABLE
#include "src/user/constants.h"

#include <memory>
#include <cstring>

#include <SFML/Graphics.hpp>
#include <SFML/Audio/SoundRecorder.hpp>

#define LMBD_SIMU_ENABLED
#define LMBD_SIMU_REALCOLORS

constexpr int ledW = stripXCoordinates;
constexpr int ledH = stripYCoordinates;

namespace {
#include "src/system/utils/utils.h"
#include "src/system/utils/colorspace.cpp"
}
using utils::map;

#include "src/system/utils/coordinates.cpp"

//
// led strip fake struct
//

struct LedStrip {
  uint8_t brightness;
  uint32_t leds[LED_COUNT];

  uint8_t _buffer8b[LED_COUNT];
  uint16_t _buffer16b[LED_COUNT];

  std::unique_ptr<sf::RectangleShape> shapes[LED_COUNT];

  LedStrip() {

    brightness = 255;
    for (size_t I = 0; I < LED_COUNT; ++I) {
      leds[I] = 0;
      shapes[I] = std::make_unique<sf::RectangleShape>();
    }
  }

  uint8_t getBrightness() {
    return brightness;
  }

  void setPixelColorXY(uint8_t X, uint8_t Y, uint8_t r, uint8_t g, uint8_t b);

  template <typename T>
  void setPixelColorXY(uint8_t X, uint8_t Y, T cc) {
    uint32_t c;
    memcpy(&c, &cc, sizeof(c));
    uint64_t r = ((c >> 16) & 0xff);
    uint64_t g = ((c >> 8) & 0xff);
    uint64_t b = (c & 0xff);
    setPixelColorXY(X, Y, r, g, b);
  }

  void setPixelColor(uint16_t led, uint8_t r, uint8_t g, uint8_t b) {

#ifndef LMBD_SIMU_REALCOLORS
    // non-linear scale for better display
    if (r < 64) r *= 4;
    if (g < 64) g *= 4;
    if (b < 64) b *= 4;
    if (r < 32) r *= 4;
    if (g < 32) g *= 4;
    if (b < 32) b *= 4;
    if (r < 16) r *= 4;
    if (g < 16) g *= 4;
    if (b < 16) b *= 4;
#endif

    leds[led] = (r << 16) | (g << 8) | b;
    return;
  }

  template <typename T>
  void setPixelColor(uint16_t led, T cc) {
    uint32_t c;
    memcpy(&c, &cc, sizeof(c));
    uint64_t r = ((c >> 16) & 0xff);
    uint64_t g = ((c >> 8) & 0xff);
    uint64_t b = (c & 0xff);
    setPixelColor(led, r, g, b);
  }

  void fadeToBlackBy(uint8_t amount) {
    for (size_t I = 0; I < LED_COUNT; ++I) {
      uint64_t b = (leds[I] & 0xff);
      uint64_t g = ((leds[I] >> 8) & 0xff);
      uint64_t r = ((leds[I] >> 16) & 0xff);

      r *= (240 - amount);
      g *= (240 - amount);
      b *= (240 - amount);

      r /= 256;
      g /= 256;
      b /= 256;

      leds[I] = (r << 16) | (g << 8) | b;
    }
  }

  void fill(uint32_t c, uint16_t start, uint16_t end) {
    for (size_t I = start; I < end; ++I) {
      leds[I] = c;
    }
  }

  void clear() {
    fill(0, 0, LED_COUNT);
  }

  static uint32_t Color(uint8_t r, uint8_t g, uint8_t b) {
    return (r << 16) | (g << 8) | b;
  }

  static uint32_t ColorHSV(double h, double s = 255, double v = 255);

  uint32_t* get_buffer_ptr(int) {
    return leds;
  }

  uint32_t getPixelColor(uint16_t idx) {
    return leds[idx];
  }

  void buffer_current_colors(int) { }; // TODO: implement
  void blur(int) { } // TODO: implement

  Cartesian get_lamp_coordinates(int i) {
    return to_lamp(i);
  }
};

//
// lampda-specific code
//

#define STRIP_H
#include "src/system/ext/noise.cpp"
#include "src/system/colors/palettes.cpp"
#include "src/system/colors/colors.cpp"
#include "src/system/colors/wipes.cpp"
#include "src/system/colors/animations.cpp"

namespace {
#include "src/system/utils/utils.cpp"
}

//
// LedStrip hacks
//

void LedStrip::setPixelColorXY(uint8_t X, uint8_t Y, uint8_t r, uint8_t g, uint8_t b) {
  setPixelColor(to_strip(X, Y), r, g, b);
}

uint32_t LedStrip::ColorHSV(double h, double s, double v) {

  int range = (int)std::floor(h / 60.0);
  double c = v * s;
  double x = c * (1 - abs(std::fmod(h / 60.0, 2) - 1.0));
  double m = v - c;

  uint8_t red = 0, green = 0, blue = 0;
  switch (range) {
    case 0:
      red = (c + m) * 255;
      green = (x + m) * 255;
      blue = m * 255;
      break;
    case 1:
      red = (x + m) * 255;
      green = (c + m) * 255;
      blue = m * 255;
      break;
    case 2:
      red = m * 255;
      green = (c + m) * 255;
      blue = (x + m) * 255;
      break;
    case 3:
      red = m * 255;
      green = (x + m) * 255;
      blue = (c + m) * 255;
      break;
    case 4:
      red = (x + m) * 255;
      green = m * 255;
      blue = (c + m) * 255;
      break;
    default:  // case 5:
      red = (c + m) * 255;
      green = m * 255;
      blue = (x + m) * 255;
      break;
  }

  return (red << 16) | (blue << 8) | green;
}

//
// basic microphone emulation
//

namespace microphone {

static constexpr float silenceLevelDb = -57;
static constexpr float highLevelDb = 80;

static float sound_level;

float get_sound_level_Db() {
  return sound_level;
}

// to hook microphone & measure levelDb
class LevelRecorder : public sf::SoundRecorder {
    virtual bool onStart() { return true; }

    virtual bool onProcessSamples(const sf::Int16* samples, std::size_t sampleCount) {
      float sumOfAll = 0;
      for(std::size_t i = 0; i < sampleCount; i++) {
          sumOfAll += powf(samples[i] / (float) 1024.0, 2.0);
	    }

      const float average = sumOfAll / (float) sampleCount;
      sound_level = 20.0 * log10f(sqrtf(average));
      return true;
    }

    virtual void onStop() { }

public:
    ~LevelRecorder() { stop(); }
};

}

using namespace microphone;

//
// simulator core loop
//

template <typename T>
struct simulator {
  static constexpr float screenWidth = 1920;
  static constexpr float screenHeight = 1080;
  static constexpr char title[] = "lampda simulator";

  static int run() {
    T simu{};

    // time
    sf::Clock clock;

    // sound
    LevelRecorder recorder;
    recorder.start();

    // fake strip
    LedStrip strip{};

    // render window
    sf::RenderWindow window(sf::VideoMode(screenWidth, screenHeight), title);

    uint64_t skipframe = 0;
    while (window.isOpen()){

      // faster fps if "F" key is pressed
      if (simu.fps < 1000) {
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::F)) {
          sf::sleep(sf::milliseconds(1000.f / (simu.fps * 3)));
        } else {
          sf::sleep(sf::milliseconds(1000.f / simu.fps));
        }
      }

      // update millis
      globalMillis = clock.getElapsedTime().asMilliseconds();

      sf::Event event;
      while (window.pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
          window.close();
          recorder.stop();
        }

        simu.customEventHandler(event);
      }

      // execute user function
      simu.loop(strip);

      if (simu.fps > 250) {
        skipframe += 1;
        if (skipframe % (((uint64_t) simu.fps) - 250) != 0)
          continue;
      }

      // draw fake leds
      window.clear();

      for (int fXpos = -simu.fakeXorigin; fXpos < ledW - simu.fakeXend; ++fXpos) {
        for (int Ypos = 0; Ypos < ledH; ++Ypos) {
          int Yoff = 0;

          int Xpos = fXpos;
          if (fXpos < 0) {
            Xpos = ledW + fXpos;
            Yoff = 1;
          }

          size_t I = to_strip(Xpos, Ypos);
          auto& shape = *strip.shapes[I];

          const float ledSz = simu.ledSizePx;
          const auto ledPadSz = simu.ledPaddingPx + simu.ledSizePx;
          const auto ledOffset = simu.ledOffsetPx;
          shape.setSize({ledSz, ledSz});

          int rXpos = fXpos + simu.fakeXorigin;
          int rYpos = Ypos + Yoff;
          shape.setPosition(ledSz + rXpos * ledPadSz + ledOffset * (Ypos % 2) - Yoff * ledOffset,
                            rYpos * ledPadSz);

          uint64_t b = (strip.leds[I] & 0xff);
          uint64_t g = ((strip.leds[I] >> 8) & 0xff);
          uint64_t r = ((strip.leds[I] >> 16) & 0xff);

          shape.setFillColor(sf::Color(r, g, b));
          window.draw(shape);
        }
      }

      window.display();
    }

    return 0;
  }

};
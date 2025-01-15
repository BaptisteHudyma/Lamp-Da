#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <Arduino.h>

// Call the main file after mocks
#include "src/system/global.h"

// see LMBD_LAMP_TYPE__INDEXABLE hard-coded in simulator/Makefile
#include "src/user/constants.h"

#include <memory>
#include <cstring>
#include <iostream>

#include <SFML/Graphics.hpp>
#include <SFML/Audio/SoundRecorder.hpp>

#define LMBD_SIMU_ENABLED
#define LMBD_SIMU_REALCOLORS
#define LMBD_LAMP_TYPE__INDEXABLE

constexpr int ledW = stripXCoordinates;
constexpr int ledH = stripYCoordinates;

#include "src/user/functions.h"

#include "simulator/include/hardware_influencer.h"

//
// simulator core loop
//

template<typename T> struct simulator
{
  static constexpr float screenWidth = 1920;
  static constexpr float screenHeight = 1080;
  static constexpr char title[] = "lampda simulator";

  // to hook microphone & measure levelDb
  class LevelRecorder : public sf::SoundRecorder
  {
    virtual bool onStart() { return true; }

    virtual bool onProcessSamples(const sf::Int16* samples, std::size_t sampleCount)
    {
      float sumOfAll = 0;
      for (std::size_t i = 0; i < sampleCount; i++)
      {
        sumOfAll += powf(samples[i] / (float)1024.0, 2.0);
      }

      const float average = sumOfAll / (float)sampleCount;
      _soundLevel = 20.0 * log10f(sqrtf(average));
      return true;
    }

    virtual void onStop() {}

  public:
    float _soundLevel;
    ~LevelRecorder() { stop(); }
  };

  inline static LevelRecorder recorder;

  static int run()
  {
    // time
    sf::Clock clock;

    globalMillis = clock.getElapsedTime().asMilliseconds();

    // sound
    if (not recorder.start())
    {
      std::cerr << "Could not start sound monitoring" << std::endl;
      return 1;
    }

    // render window
    sf::RenderWindow window(sf::VideoMode(screenWidth, screenHeight), title);

    // build simu
    T simu;

    // Main program setup
    global::main_setup();

    uint64_t skipframe = 0;
    while (window.isOpen())
    {
      // update time_ms()
      globalMillis = clock.getElapsedTime().asMilliseconds();

      // update the callbacks of the gpios (simulate interrupts)
      mock_gpios::update_callbacks();

      // deep sleep, exit
      if (mock_registers::isDeepSleep)
        break;

      // other events
      sf::Event event;
      while (window.pollEvent(event))
      {
        if (event.type == sf::Event::Closed)
        {
          window.close();
          recorder.stop();
        }
      }

      // main program loop
      global::main_loop();

      float dt = clock.getElapsedTime().asMilliseconds() - globalMillis;

      // re-execute immediately if faster fps needed
      if (simu.fps > 250)
      {
        skipframe += 1;
        if (skipframe % (((uint64_t)simu.fps) - 250) != 0)
          continue;
      }

      // faster fps if "F" key is pressed
      if (simu.fps < 1000 && dt < 1000.f && simu.fps > 0)
      {
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::F))
        {
          sf::sleep(sf::milliseconds((1000.f - dt) / (simu.fps * 3)));
        }
        else
        {
          sf::sleep(sf::milliseconds((1000.f - dt) / simu.fps));
        }
      }

      // draw fake leds
      window.clear();

      for (int fXpos = -simu.fakeXorigin; fXpos < ledW - simu.fakeXend; ++fXpos)
      {
        for (int Ypos = 0; Ypos < ledH; ++Ypos)
        {
          int Yoff = 0;

          int Xpos = fXpos;
          if (fXpos < 0)
          {
            Xpos = ledW + fXpos;
            Yoff = 1;
          }

          size_t I = to_strip(Xpos, Ypos);
          auto& shape = *user::_private::strip.shapes[I];

          const float ledSz = simu.ledSizePx;
          const auto ledPadSz = simu.ledPaddingPx + simu.ledSizePx;
          const auto ledOffset = simu.ledOffsetPx;
          shape.setSize({ledSz, ledSz});

          int rXpos = fXpos + simu.fakeXorigin;
          int rYpos = Ypos + Yoff;
          shape.setPosition(ledSz + rXpos * ledPadSz + ledOffset * (Ypos % 2) - Yoff * ledOffset, rYpos * ledPadSz);

          const uint32_t color = user::_private::strip.getPixelColor(I);
          const uint8_t brightnes = user::_private::strip.getBrightness();
          float b = (color & 0xff);
          float g = ((color >> 8) & 0xff);
          float r = ((color >> 16) & 0xff);

          r = min(r, brightnes);
          g = min(g, brightnes);
          b = min(b, brightnes);

          shape.setFillColor(sf::Color(r, g, b));
          window.draw(shape);
        }
      }

      window.display();
    }

    return 0;
  }
};

#endif
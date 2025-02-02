#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "parameter_parser.h"

// Call the main file after mocks
#include "src/system/global.h"

// see LMBD_LAMP_TYPE__INDEXABLE hard-coded in simulator/Makefile
#include "src/user/constants.h"

#include <SFML/Graphics/Color.hpp>
#include <memory>
#include <cstring>
#include <iostream>

#include <SFML/Graphics.hpp>

#define LMBD_SIMU_ENABLED
#define LMBD_SIMU_REALCOLORS
#define LMBD_LAMP_TYPE__INDEXABLE

constexpr int ledW = stripXCoordinates;
constexpr int ledH = stripYCoordinates;

#include "src/user/functions.h"
#include "src/system/utils/utils.h"

#include "simulator/include/hardware_influencer.h"

#include <thread>

//
// simulator core loop
//

template<typename T> struct simulator
{
  static constexpr float screenWidth = 1920;
  static constexpr float screenHeight = 1080;
  static constexpr char title[] = "lampda simulator";

  static int run()
  {
    // time
    sf::Clock clock;

    // render window
    sf::RenderWindow window(sf::VideoMode(screenWidth, screenHeight), title);

    sf::CircleShape indicator(60);
    sf::CircleShape buttonMask(40);
    buttonMask.setFillColor(sf::Color::Black);

    // load initial values
    read_and_update_parameters();

    // build simu
    T simu;

    // Main program setup
    global::main_setup();

    // mock run the threads (do not give each subprocess a thread)
    mock_registers::shouldStopThreads = false;
    std::thread mockThreads(&mock_registers::run_threads);

    uint64_t skipframe = 0;
    while (window.isOpen())
    {
      // read events, prevent windows lockup
      sf::Event event;
      while (window.pollEvent(event))
      {
#if 0
        if (event.type == sf::Event::Closed)
        {
          window.close();
        }
#endif
      }

      // deep sleep, exit
      if (mock_registers::isDeepSleep)
        break;

      // update simu parameters
      read_and_update_parameters();
      // update the callbacks of the gpios (simulate interrupts)
      mock_gpios::update_callbacks();

      // main program loop
      global::main_loop(mock_registers::addedAlgoDelay);

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

      // display the indicator button
      const uint32_t indicatorColor = mock_indicator::get_color();
      float b = (indicatorColor & 0xff);
      float g = ((indicatorColor >> 8) & 0xff);
      float r = ((indicatorColor >> 16) & 0xff);
      indicator.setFillColor(sf::Color(r, g, b));
      const auto coordX = simu.ledSizePx * ledW / 2.0;
      const auto coordY = simu.ledSizePx * (ledH + 4);
      indicator.setPosition(coordX, coordY);
      buttonMask.setPosition(coordX + buttonMask.getRadius() / 2, coordY + buttonMask.getRadius() / 2);
      window.draw(indicator);
      window.draw(buttonMask);

      window.display();
    }

    window.close();
    // close all threads
    mock_registers::shouldStopThreads = true;
    mockThreads.join();

    return 0;
  }
};

#endif
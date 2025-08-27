#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <cstring>
#include <iostream>

// include this first
#include "src/system/global.h"

#include "parameter_parser.h"
#include "simulator_state.h"

// see LMBD_LAMP_TYPE__${UPPER_SIM_NAME} hard-coded in simulator/Makefile
#include "src/user/constants.h"

#include <SFML/Graphics.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/Window/Keyboard.hpp>

#define LMBD_SIMU_REALCOLORS

// (_LampTy from simulator_state.h)
constexpr int ledW = _LampTy::maxWidth;
constexpr int ledH = _LampTy::maxHeight;

constexpr float fLedW = _LampTy::maxWidthFloat;
constexpr float fResidueW = 1 / (2 * fLedW - 2 * floor(fLedW) - 1);

#include "src/user/functions.h"
#include "src/system/utils/utils.h"

#include "simulator/include/hardware_influencer.h"

#include <thread>

//
// simulator core loop
//

template<typename T> struct simulator
{
  static constexpr uint screenWidth = 1920;
  static constexpr uint screenHeight = 1080;
  static constexpr char title[] = "lampda simulator";

  static constexpr uint brightnessWidth = 480;

  static int run()
  {
    T simu;
    sf::Clock time;

    // brightness tracker
    std::array<uint16_t, brightnessWidth> brightnessTracker {};
    std::array<sf::RectangleShape, brightnessWidth> dots;

    // pixel shapes
    std::array<sf::RectangleShape, _LampTy::ledCount> shapes;

    // init font
    sf::Font font;
    bool enableFont = false, enableText = false;
    if (font.openFromFile("simulator/resources/DejaVuSansMono.ttf"))
    {
      enableFont = true;
    }
    else
    {
      fprintf(stderr, "DejaVuSansMono.ttf not found, disabling text...");
    }

    // render window
    sf::RenderWindow window(sf::VideoMode({screenWidth, screenHeight}), title);

    sf::CircleShape buttonMask(simu.buttonSize);
    sf::CircleShape indicator(simu.buttonSize + simu.buttonMargin);
    buttonMask.setFillColor(sf::Color::Black);

    sf::Vector2<int> mousePos = sf::Mouse::getPosition(window);
    const float indCoordX = simu.buttonLeftPos;
    const float indCoordY = simu.ledSizePx * (ledH + 6);

    // TODO: #156 move mock-specific parts to another process
    // ======================================================

    // load initial values
    read_and_update_parameters();

    // Main program setup
    global::main_setup();

    // mock run the threads (do not give each subprocess a thread)
    mock_registers::shouldStopThreads = false;
    std::thread mockThreads(&mock_registers::run_threads);

    // =================================================

    // reference to global state
    auto& state = sim::globals::state;

    uint64_t skipframe = 0;
    while (window.isOpen())
    {
      uint64_t mouseClick = 0;

      mousePos = sf::Mouse::getPosition(window);
      enableText = enableFont && sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
      enableText |= state.verbose;

      // read events, prevent windows lockup
      while (const std::optional event = window.pollEvent())
      {
        if (event->is<sf::Event::Closed>())
        {
          window.close();
          break;
        }

        if (event->is<sf::Event::KeyPressed>())
        {
          auto kpressed = event->getIf<sf::Event::KeyPressed>();
          switch (kpressed->code)
          {
            default:
              state.lastKeyPressed = 0;
              break;
            case sf::Keyboard::Key::P: // p == pause
              state.lastKeyPressed = 'p';
              break;
            case sf::Keyboard::Key::F: // f == faster
              state.lastKeyPressed = 'f';
              break;
            case sf::Keyboard::Key::S: // s == slower
              state.lastKeyPressed = 's';
              break;
            case sf::Keyboard::Key::V: // v == verbose
              state.lastKeyPressed = 'v';
              break;
            case sf::Keyboard::Key::T: // t == tick +pause
              state.lastKeyPressed = 't';
              break;
            case sf::Keyboard::Key::W:
              state.lastKeyPressed = 'w';
              break;
            case sf::Keyboard::Key::A:
              state.lastKeyPressed = 'a';
              break;
            case sf::Keyboard::Key::D:
              state.lastKeyPressed = 'd';
              break;
            case sf::Keyboard::Key::U:
              state.lastKeyPressed = 'u';
              break;
            case sf::Keyboard::Key::I:
              state.lastKeyPressed = 'i';
              break;
            case sf::Keyboard::Key::R:
              state.lastKeyPressed = 'r';
              break;
            case sf::Keyboard::Key::G:
              state.lastKeyPressed = 'g';
              break;
            case sf::Keyboard::Key::C:
              state.lastKeyPressed = 'c';
              break;
            case sf::Keyboard::Key::Q:
              state.lastKeyPressed = 'q';
              break;
          }

          break;
        }

        if (event->is<sf::Event::KeyReleased>())
        {
          // tick forward (once) and pause
          if (state.lastKeyPressed == 't')
          {
            state.paused = false;
            state.tickAndPause = 2;
          }

          // pause event
          if (state.lastKeyPressed == 'p')
          {
            state.paused = !state.paused;
          }

          // verbose event
          if (state.lastKeyPressed == 'v')
          {
            state.verbose = !state.verbose;
          }

          // slower simulation
          if (state.lastKeyPressed == 's')
          {
            if (state.slowTimeFactor > 1.50)
            {
              state.slowTimeFactor *= 0.95;
            }
            else if (state.slowTimeFactor > 1.0)
            {
              state.slowTimeFactor = ceil(state.slowTimeFactor * 20 - 1) / 20;
            }
            else
            {
              state.slowTimeFactor = max(0.1, state.slowTimeFactor - 0.05);
            }
            fprintf(stderr, "slower %f\n", state.slowTimeFactor);
          }

          // faster simulation
          if (state.lastKeyPressed == 'f')
          {
            if (state.slowTimeFactor > 1.20)
            {
              state.slowTimeFactor *= 1.05;
            }
            else if (state.slowTimeFactor > 1.0)
            {
              state.slowTimeFactor = ceil(state.slowTimeFactor * 20 + 1) / 20;
            }
            else
            {
              state.slowTimeFactor += 0.05;
            }
            fprintf(stderr, "faster %f\n", state.slowTimeFactor);
          }

          // reset key pressed
          state.lastKeyPressed = 0;
          break;
        }

        if (event->is<sf::Event::MouseButtonPressed>())
        {
          float dx = indCoordX - mousePos.x + simu.buttonSize;
          float dy = indCoordY - mousePos.y + simu.buttonSize;
          float norm = simu.buttonSize;

          if (dx * dx + dy * dy < norm * norm)
          {
            mouseClick += 3;
          }
          break;
        }
      }

      // update simulator global state
      state.isButtonPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space);
      if (mouseClick > 0)
      {
        mouseClick -= 1;
        state.isButtonPressed = true;
      }

      if (state.tickAndPause > 0)
      {
        if (state.tickAndPause == 1)
        {
          state.paused = true;
        }
        state.tickAndPause -= 1;
      }

      // TODO: #156 move mock-specific parts to another process
      // ======================================================

      if (!state.paused)
      {
        // deep sleep, exit
        if (mock_registers::isDeepSleep)
        {
          // close all threads
          mock_registers::shouldStopThreads = true;
          mockThreads.join();
          window.close();
          break;
        }

        // update simu parameters
        read_and_update_parameters();
        // update the callbacks of the gpios (simulate interrupts)
        mock_gpios::update_callbacks();

        // main program loop
        global::main_loop(mock_registers::addedAlgoDelay);
      }

      if constexpr (_LampTy::flavor == modes::hardware::LampTypes::indexable)
      {
        for (size_t I = 0; I < _LampTy::ledCount; ++I)
        {
#ifdef LMBD_LAMP_TYPE__INDEXABLE
          state.colorBuffer[I] = user::_private::strip.getPixelColor(I);
          state.brightness = user::_private::strip.getBrightness();
#endif
        }
      }
      else
      {
        for (size_t I = 0; I < _LampTy::ledCount; ++I)
          state.colorBuffer[I] = 0xffff00;
        state.brightness = (brightness::get_brightness() * 255) / maxBrightness;
      }

      state.indicatorColor = mock_indicator::get_color();
      // ======================================================

      // draw fake leds
      window.clear(sf::Color::Black);

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

          size_t I = modes::to_strip(Xpos, Ypos);
          auto& shape = shapes[I];

          const float ledSz = simu.ledSizePx;
          const auto ledPadSz = simu.ledPaddingPx + simu.ledSizePx;
          const auto ledOffset = simu.ledOffsetPx;
          shape.setSize({ledSz, ledSz});

          int rXpos = fXpos + simu.fakeXorigin;
          int rYpos = Ypos + Yoff;
          int shiftR = Ypos / fResidueW;
          int shiftL = Ypos + shiftR + 1;

          shape.setPosition({ledSz + rXpos * ledPadSz + ledOffset * (shiftL % 2) - (Yoff - shiftR) * ledOffset,
                             rYpos * ledPadSz});

          const uint32_t color = state.colorBuffer[I];
          float b = (color & 0xff);
          float g = ((color >> 8) & 0xff);
          float r = ((color >> 16) & 0xff);

          const auto brightness = state.brightness;
          r = min(r, brightness);
          g = min(g, brightness);
          b = min(b, brightness);

          shape.setFillColor(sf::Color(r, g, b));
          window.draw(shape);

          // display text information if mouse is pressed
          if (enableText)
          {
            // (used to compute which pixel is pointed by mouse)
            int MouseYpos = 1 + (mousePos.y / ledPadSz);
            shiftR = MouseYpos / fResidueW;
            shiftL = MouseYpos + shiftR + 1;
            float MouseXpos = ((mousePos.x - ledSz - (shiftL % 2 - Yoff + shiftR) * ledOffset) / ledPadSz);

            if (MouseYpos > ledH + 1 && !state.verbose)
            {
              enableText = false;
              continue;
            }

            float offsetY = 0;
            if (MouseXpos < simu.fakeXorigin - 0.5)
            {
              offsetY = ledSz;
              MouseYpos += 1;
            }

            // display Ypos on the right
            float dy = abs(shape.getPosition().y - mousePos.y + ledSz / 2 + offsetY);
            if (fXpos + 1 >= ledW - simu.fakeXend && dy < ledPadSz / 2)
            {
              sf::Text text(font, std::to_string(Ypos));
              text.setCharacterSize(12);

              sf::Vector2f where(ledSz + (ledW + simu.fakeXorigin) * ledPadSz + ledOffset * 2, rYpos * ledPadSz + 6);
              text.setPosition(where);
              window.draw(text);
            }

            // display Xpos on the bottom
            int shiftR = MouseYpos / fResidueW;
            int shiftL = MouseYpos + shiftR + 1;
            float dx = abs(shape.getPosition().x - mousePos.x - (MouseYpos % 2) * ledOffset + ledSz);
            if (Ypos + 1 >= ledH && dx < ledPadSz / 2)
            {
              sf::Text text(font, std::to_string(Xpos));
              text.setCharacterSize(12);

              sf::Vector2f where(ledSz + rXpos * ledPadSz - Yoff * ledOffset + 16, (ledH + Yoff) * ledPadSz + 8);
              text.setPosition(where);
              window.draw(text);
            }

            // display color at pixel
            if (dx < ledPadSz / 2 && dy < ledPadSz / 2)
            {
              auto str = "(" + std::to_string(int(r)) + ", ";
              str += std::to_string(int(g)) + ", ";
              str += std::to_string(int(b)) + ")";

              sf::Text text(font, str);
              text.setCharacterSize(12);

              sf::Vector2f where(indCoordX + simu.buttonSize * 3, indCoordY - 16.f);
              text.setPosition(where);
              window.draw(text);
            }
          }
        }
      }

      // display the indicator button
      const uint32_t indicatorColor = state.indicatorColor;
      float b = (indicatorColor & 0xff);
      float g = ((indicatorColor >> 8) & 0xff);
      float r = ((indicatorColor >> 16) & 0xff);
      indicator.setFillColor(sf::Color(r, g, b));
      indicator.setPosition({indCoordX, indCoordY});
      buttonMask.setPosition({indCoordX + simu.buttonMargin, indCoordY + simu.buttonMargin});
      window.draw(indicator);
      window.draw(buttonMask);

      // track brightness & display dot curve
      float now = (time.getElapsedTime().asMilliseconds() * simu.fps) / 1000.f;
      uint32_t trackerPos = now / simu.brightnessRate;

      trackerPos = trackerPos % brightnessTracker.size();
      brightnessTracker[trackerPos] = state.brightness;
      brightnessTracker[(trackerPos + 1) % brightnessTracker.size()] = 0;

      float xDotsOrigin = indCoordX + simu.buttonSize * 2 + simu.buttonMargin * 2 + 16.f;
      float yDotsOrigin = indCoordY + simu.brightnessScale;

      for (size_t I = 0; I < brightnessTracker.size(); ++I)
      {
        auto& dot = dots[I];

        float v = (brightnessTracker[I] * simu.brightnessScale) / 256;
        float x = xDotsOrigin + I;
        float y = yDotsOrigin - v;
        dot.setSize({1, 1});
        dot.setPosition({x, y});
        dot.setFillColor(sf::Color(0, 0xff, 0));
        window.draw(dot);
      }

      // draw window
      window.display();
    }

    return 0;
  }
};

#endif

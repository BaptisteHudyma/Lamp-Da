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
constexpr int ledH = _LampTy::maxOverflowHeight;
constexpr int ledCount = _LampTy::ledCount;

constexpr float fLedW = _LampTy::maxWidthFloat;
constexpr float fResidueW = 1 / (2 * fLedW - 2 * floor(fLedW) - 1);

#include "src/user/functions.h"
#include "src/system/utils/utils.h"

#include "simulator/include/hardware_influencer.h"
// handle simulation of voltage and current in the system
#include "simulator/mocks/electrical/electrical_mock.cpp"

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

    // matrix shift
    int fakeXorigin = 0;
    int fakeXend = 0;

    // pixel shapes
    std::array<sf::RectangleShape, _LampTy::ledCount> shapes;

    // init font
    sf::Font font;
    bool enableFont = false, enableText = false;
    if (font.loadFromFile("simulator/resources/DejaVuSansMono.ttf"))
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
    const float indCoordY = (simu.ledSizePx + simu.ledPaddingPx) * (ledH + 3);

    // TODO: #156 move mock-specific parts to another process
    // ======================================================

    // mock run the threads (do not give each subprocess a thread)
    mock_registers::shouldStopThreads = false;

    // load initial values
    read_and_update_parameters();
    // start electrical simulation,
    start_electrical_mock();

    // force init of time clock
    time_ms();
    // Main program setup
    global::main_setup();

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
      sf::Event event;
      while (window.pollEvent(event))
      {
        if (event.type == sf::Event::Closed)
        {
          window.close();
          break;
        }

        if (event.type == sf::Event::KeyPressed)
        {
          auto kpressed = event.key.code;
          switch (kpressed)
          {
            default:
              state.lastKeyPressed = 0;
              break;
            case sf::Keyboard::Key::P: // p == pause
              state.lastKeyPressed = 'p';
              break;
            case sf::Keyboard::Key::V: // v == verbose
              state.lastKeyPressed = 'v';
              break;
            case sf::Keyboard::Key::T: // t == tick +pause
              state.lastKeyPressed = 't';
              break;
            case sf::Keyboard::Key::H: // h == higher speed
              state.lastKeyPressed = 'h';
              break;
            case sf::Keyboard::Key::G: // g == glower speeds
              state.lastKeyPressed = 'g';
              break;
            case sf::Keyboard::Key::J: // j == shift matrix left
              state.lastKeyPressed = 'j';
              break;
            case sf::Keyboard::Key::K: // k == shift matrix right
              state.lastKeyPressed = 'k';
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
            case sf::Keyboard::Key::C:
              state.lastKeyPressed = 'c';
              break;
            case sf::Keyboard::Key::Q:
              state.lastKeyPressed = 'q';
              break;
          }

          break;
        }

        if (event.type == sf::Event::KeyReleased)
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
          if (state.lastKeyPressed == 'g')
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
          if (state.lastKeyPressed == 'h')
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

          // shift forward XY display
          if (state.lastKeyPressed == 'k')
          {
            fakeXorigin += 1;
            if (fakeXorigin > ledW - 1)
              fakeXorigin = 0;
            fakeXend = max(fakeXorigin - min(4, ledW - 1 - fakeXorigin), 0);
          }

          // shift backward XY display
          if (state.lastKeyPressed == 'j')
          {
            fakeXorigin -= 1;
            if (fakeXorigin < 0)
              fakeXorigin = ledW - 1;
            fakeXend = max(fakeXorigin - min(4, ledW - 1 - fakeXorigin), 0);
          }

          // reset key pressed
          state.lastKeyPressed = 0;
          break;
        }

        if (event.type == sf::Event::MouseButtonPressed)
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

      const bool isOutputEnabled = is_output_enabled();
      if constexpr (_LampTy::flavor == modes::hardware::LampTypes::indexable)
      {
        const bool isVoltageHighEnough = mock_electrical::outputVoltage > 11.5;
        for (size_t I = 0; I < _LampTy::ledCount; ++I)
        {
#ifdef LMBD_LAMP_TYPE__INDEXABLE
          state.colorBuffer[I] =
                  isOutputEnabled ? (isVoltageHighEnough ? user::_private::strip.getPixelColor(I) : 0xffffff) : 0;
          state.brightness = user::_private::strip.getBrightness();
#endif
        }
      }
      else
      {
        for (size_t I = 0; I < _LampTy::ledCount; ++I)
          state.colorBuffer[I] = isOutputEnabled ? 0xffff00 : 0;
        state.brightness = (brightness::get_brightness() * 255) / maxBrightness;
      }

      state.indicatorColor = mock_indicator::get_color();
      // ======================================================

      const float ledSz = simu.ledSizePx;
      const auto ledPadSz = simu.ledPaddingPx + simu.ledSizePx;
      const auto ledOffset = ledSz / _LampTy::shiftPeriod;

      const float Xbase = _LampTy::extraShiftTotal > 0 ? 0 : -_LampTy::extraShiftTotal;
      const float Xextra = _LampTy::extraShiftTotal < 0 ? 0 : _LampTy::extraShiftTotal;

      // draw slightly grey background to hightlight turned off LEDs
      window.clear(sf::Color(13, 12, 11));

      for (int Ypos = 0; Ypos < ledH; ++Ypos)
      {
        int realRowSize = ledW;
        if (_LampTy::allDeltaResiduesY[Ypos])
          realRowSize += 1;

        for (int fXpos = -fakeXorigin; fXpos < realRowSize - fakeXend; ++fXpos)
        {
          int Yoff = 0, Xoff = 0;

          int Xpos = fXpos;
          if (fXpos < 0)
          {
            Xpos = realRowSize + fXpos;
            Yoff = 1;

            if (_LampTy::allDeltaResiduesY[Ypos])
              Xoff = 1;
          }

          size_t I = modes::to_strip(Xpos, Ypos);
          auto& shape = shapes[I];

          auto realPos = modes::strip_to_XY(I);
          if (realPos.x != Xpos || realPos.y != Ypos)
            continue;

          shape.setSize({ledSz, ledSz});

          int rXpos = fXpos + fakeXorigin + Xoff;
          int rYpos = Ypos + Yoff;
          int shiftR = _LampTy::extraShiftResiduesY[Ypos];
          int shiftL = Ypos + shiftR + 1;

          if (_LampTy::shiftResidue == 1 && _LampTy::shiftPerTurn < 0.1)
            shiftR = 0;

          float shapeX = ledSz + rXpos * ledPadSz;
          shapeX += ledOffset * Xbase;
          shapeX += ledOffset * (shiftL % _LampTy::shiftPeriod);
          shapeX -= (ledOffset - simu.ledPaddingPx / 2) * (Yoff - shiftR);
          shapeX -= Xoff * simu.ledPaddingPx;

          // make the origin more obvious
          if (fXpos >= 0)
            shapeX += simu.ledPaddingPx;

          float shapeY = rYpos * ledPadSz;
          shape.setPosition({shapeX, shapeY});

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
            int MouseYpos = mousePos.y;
            int MouseXpos = mousePos.x;

            auto shapeXY = shape.getPosition();
            float dx = shapeXY.x + ledSz / 2 - MouseXpos;
            float dy = shapeXY.y + ledSz / 2 - MouseYpos;

            // all shapes too far from cursor, does not display info
            if (abs(2 * dx) > ledPadSz || abs(2 * dy) > ledPadSz)
              continue;

            // display (X,Y) on bottom-right
            float largestX = ledSz + (ledW + 1 + fakeXorigin - fakeXend) * ledPadSz + ledOffset * (Xextra + Xbase);
            float largestY = (ledH + Yoff) * ledPadSz + 8;
            {
              auto str = "(" + std::to_string(Xpos) + ", " + std::to_string(Ypos) + ")";

              sf::Text text(str, font, 12);
              sf::Vector2f where(largestX, largestY);
              text.setPosition(where);
              window.draw(text);
            }

            // display Ypos on the right
            {
              auto str = std::to_string(Ypos);
              if (_LampTy::allDeltaResiduesY[Ypos])
                str += " *";

              sf::Text text(str, font, 12);
              sf::Vector2f where(largestX, shapeXY.y + ledSz / 4);
              text.setPosition(where);
              window.draw(text);
            }

            // display Xpos on the bottom
            {
              auto str = std::to_string(Xpos);
              if (Xpos == ledW)
                str += " *";

              sf::Text text(str, font, 12);
              sf::Vector2f where(shapeXY.x, largestY);
              text.setPosition(where);
              window.draw(text);
            }

            // display misc info next to button
            {
              auto str = "(" + std::to_string(int(r)) + ", ";
              str += std::to_string(int(g)) + ", ";
              str += std::to_string(int(b)) + ")";
              str += ", " + std::to_string(I);

              sf::Text text(str, font, 12);
              sf::Vector2f where(indCoordX + simu.buttonSize * 3, indCoordY - 24.f);
              text.setPosition(where);
              window.draw(text);

              // (avoid superposed text)
              enableText = false;
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

    stop_electrical_mock();

    return 0;
  }
};

#endif

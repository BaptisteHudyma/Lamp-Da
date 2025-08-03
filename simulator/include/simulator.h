#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "parameter_parser.h"
#include "simulator_state.h"

// Call the main file after mocks
#include "src/system/global.h"

// see LMBD_LAMP_TYPE__${UPPER_SIM_NAME} hard-coded in simulator/Makefile
#include "src/user/constants.h"

#include <SFML/Graphics.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <memory>
#include <cstring>
#include <iostream>

#include <SFML/Graphics.hpp>

#define LMBD_SIMU_ENABLED
#define LMBD_SIMU_REALCOLORS

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
  static constexpr uint screenWidth = 1920;
  static constexpr uint screenHeight = 1080;
  static constexpr char title[] = "lampda simulator";

  static int run()
  {
    // init shapes
    std::unique_ptr<sf::RectangleShape> shapes[LED_COUNT];
    for (size_t I = 0; I < LED_COUNT; ++I)
    {
      shapes[I] = std::make_unique<sf::RectangleShape>();
    }

    // init font
    sf::Font font;
    bool enableFont = false, enableText = false;
    if (font.openFromFile("DejaVuSansMono.ttf"))
    {
      enableFont = true;
    }
    else
    {
      fprintf(stderr, "DejaVuSansMono.ttf not found, disabling text...");
    }

    // render window
    sf::RenderWindow window(sf::VideoMode({screenWidth, screenHeight}), title);

    // build simu
    T simu;

    sf::CircleShape buttonMask(simu.buttonSize);
    sf::CircleShape indicator(simu.buttonSize + simu.buttonMargin);
    buttonMask.setFillColor(sf::Color::Black);

    sf::Vector2<int> mousePos = sf::Mouse::getPosition(window);
    const float indCoordX = simu.ledSizePx * ledW / 2.0;
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

    uint64_t skipframe = 0;
    while (window.isOpen())
    {
      uint64_t mouseClick = 0;

      mousePos = sf::Mouse::getPosition(window);
      enableText = enableFont && sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
      enableText |= sim::globals::state.verbose;

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
          auto& state = sim::globals::state;
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
          auto& state = sim::globals::state;

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
      sim::globals::state.isButtonPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space);
      if (mouseClick > 0)
      {
        mouseClick -= 1;
        sim::globals::state.isButtonPressed = true;
      }

      // TODO: #156 move mock-specific parts to another process
      // ======================================================

      if (!sim::globals::state.paused)
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

      for (size_t I = 0; I < LED_COUNT; ++I)
      {
        sim::globals::state.colorBuffer[I] = user::_private::strip.getPixelColor(I);
        sim::globals::state.brightness = user::_private::strip.getBrightness();
      }

      sim::globals::state.indicatorColor = mock_indicator::get_color();
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

          size_t I = to_strip(Xpos, Ypos);
          auto& shape = *shapes[I];

          const float ledSz = simu.ledSizePx;
          const auto ledPadSz = simu.ledPaddingPx + simu.ledSizePx;
          const auto ledOffset = simu.ledOffsetPx;
          shape.setSize({ledSz, ledSz});

          int rXpos = fXpos + simu.fakeXorigin;
          int rYpos = Ypos + Yoff;
          shape.setPosition({ledSz + rXpos * ledPadSz + ledOffset * (Ypos % 2) - Yoff * ledOffset, rYpos * ledPadSz});

          const uint32_t color = sim::globals::state.colorBuffer[I];
          float b = (color & 0xff);
          float g = ((color >> 8) & 0xff);
          float r = ((color >> 16) & 0xff);

          const auto brightness = sim::globals::state.brightness;
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
            float MouseXpos = ((mousePos.x - ledSz - (MouseYpos % 2 - Yoff) * ledOffset) / ledPadSz);

            if (MouseYpos > ledH + 1 && !sim::globals::state.verbose)
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
            float dx = abs(shape.getPosition().x - mousePos.x - (MouseYpos % 2) * ledOffset + ledSz / 2);
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

              sf::Vector2f where(indCoordX + simu.buttonSize * 3, indCoordY);
              text.setPosition(where);
              window.draw(text);
            }
          }
        }
      }

      // display the indicator button
      const uint32_t indicatorColor = sim::globals::state.indicatorColor;
      float b = (indicatorColor & 0xff);
      float g = ((indicatorColor >> 8) & 0xff);
      float r = ((indicatorColor >> 16) & 0xff);
      indicator.setFillColor(sf::Color(r, g, b));
      indicator.setPosition({indCoordX, indCoordY});
      buttonMask.setPosition({indCoordX + simu.buttonMargin, indCoordY + simu.buttonMargin});
      window.draw(indicator);
      window.draw(buttonMask);

      window.display();
    }

    return 0;
  }
};

#endif

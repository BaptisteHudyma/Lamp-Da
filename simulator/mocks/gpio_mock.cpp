/*! \file gpio_mock.cpp
    \brief Mock of the board gpio
*/

#include <map>

#include "simulator_state.h"

#include "src/system/platform/gpio.h"
#include "src/system/platform/print.h"

#include "src/system/utils/input_output.h"
#include "src/system/utils/utils.h"

#include "src/system/physical/button.h"
#include "src/system/physical/indicator.h"

#include "simulator/include/hardware_influencer.h"

#define PLATFORM_GPIO_CPP

typedef void (*voidFuncPtr)(void);

namespace simulator {

namespace mock_gpios {

inline static std::map<::lampda::platform::gpio::DigitalPin::GPIO, voidFuncPtr> callbacksRisingEdge;
inline static std::map<::lampda::platform::gpio::DigitalPin::GPIO, voidFuncPtr> callbacksFallingEdge;
inline static std::map<::lampda::platform::gpio::DigitalPin::GPIO, voidFuncPtr> callbacksChange;

static bool isButtonPressed = false;

// only update button :/ thats bad
void update_callbacks()
{
  static bool wasButtonPressed = false;

  isButtonPressed = simulator::globals::state.isButtonPressed;

  // event on change
  if (isButtonPressed != wasButtonPressed)
  {
    const ::lampda::platform::gpio::DigitalPin::GPIO buttonPin = ::lampda::physical::button::get_button_pin();
    // change always called
    for (const auto& [pin, callback]: callbacksChange)
    {
      // only handle button pin callbacks
      if (pin != buttonPin)
        continue;
      callback();
    }

    if (isButtonPressed)
    {
      for (const auto& [pin, callback]: callbacksRisingEdge)
      {
        // only handle button pin callbacks
        if (pin != buttonPin)
          continue;
        callback();
      }
    }
    else
    {
      for (const auto& [pin, callback]: callbacksFallingEdge)
      {
        // only handle button pin callbacks
        if (pin != buttonPin)
          continue;
        callback();
      }
    }
  }

  // update
  wasButtonPressed = isButtonPressed;
}

} // namespace mock_gpios

namespace mock_indicator {
::lampda::COLOR idColor;
uint32_t get_color() { return idColor.color; }
} // namespace mock_indicator

namespace mock_battery {
float voltage;
} // namespace mock_battery

// store output values
std::array<uint8_t, 255> pinOutputValue;

} // namespace simulator

namespace lampda {
namespace platform {
namespace gpio {

class DigitalPinImpl
{
public:
  DigitalPinImpl(platform::gpio::DigitalPin::GPIO pin) : _pin(pin) {}
  ~DigitalPinImpl() = default;

  bool is_high() const
  {
    // button pin
    if (_pin == physical::button::get_button_pin())
    {
      return !simulator::mock_gpios::isButtonPressed;
    }

    return simulator::pinOutputValue[static_cast<uint8_t>(_pin)] >= 128;
  }

  void set_high(bool isHigh) const { simulator::pinOutputValue[static_cast<uint8_t>(_pin)] = isHigh ? 255 : 0; }

  void write(uint16_t value) const
  {
    simulator::pinOutputValue[static_cast<uint8_t>(_pin)] = value;

    switch (_pin)
    {
      case utils::RedIndicator:
        {
          simulator::mock_indicator::idColor.red = (value & 255) / physical::indicator::redColorCorrection;
          break;
        }
      case utils::GreenIndicator:
        {
          // correction value for the real luminosity
          simulator::mock_indicator::idColor.green = (value & 255) / physical::indicator::greenColorCorrection;
          break;
        }
      case utils::BlueIndicator:
        {
          simulator::mock_indicator::idColor.blue = (value & 255) / physical::indicator::blueColorCorrection;
          break;
        }
    }
  }

  uint16_t read() const { return simulator::pinOutputValue[static_cast<uint8_t>(_pin)]; }

  void attach_callback(voidFuncPtr cllbk, platform::gpio::DigitalPin::Interrupt mode) const
  {
    // cannot have two interrupt callback types
    simulator::mock_gpios::callbacksChange.erase(_pin);
    simulator::mock_gpios::callbacksRisingEdge.erase(_pin);
    simulator::mock_gpios::callbacksFallingEdge.erase(_pin);

    switch (mode)
    {
      case DigitalPin::Interrupt::kFallingEdge:
        simulator::mock_gpios::callbacksFallingEdge[_pin] = cllbk;
        break;

      case DigitalPin::Interrupt::kRisingEdge:
        simulator::mock_gpios::callbacksRisingEdge[_pin] = cllbk;
        break;

      case DigitalPin::Interrupt::kChange:
        simulator::mock_gpios::callbacksChange[_pin] = cllbk;
        break;

      default:
        break;
    }
  }

  void detach_callbacks() const
  {
    simulator::mock_gpios::callbacksChange.erase(_pin);
    simulator::mock_gpios::callbacksRisingEdge.erase(_pin);
    simulator::mock_gpios::callbacksFallingEdge.erase(_pin);
  }

public:
  DigitalPin::GPIO _pin;
};

DigitalPin::DigitalPin(DigitalPin::GPIO pin) : mGpio(pin) { set(pin); }

void DigitalPin::set(DigitalPin::GPIO pin)
{
  mGpio = pin;
  mImpl = std::make_shared<DigitalPinImpl>(pin);
}

void DigitalPin::set_pin_mode(Mode mode) const {}

bool DigitalPin::is_high() const { return mImpl->is_high(); }

void DigitalPin::set_high(bool is_high) const { return mImpl->set_high(is_high); }

void DigitalPin::write(uint16_t value) const { mImpl->write(value); }

uint16_t DigitalPin::read() const { return mImpl->read(); }

int DigitalPin::pin() const { return 0; }

void DigitalPin::attach_callback(voidFuncPtr func, Interrupt mode) const
{
  DigitalPin::s_gpiosWithInterrupts.emplace(mGpio);
  mImpl->attach_callback(func, mode);
}

void DigitalPin::detach_callbacks() const
{
  DigitalPin::s_gpiosWithInterrupts.erase(mGpio);
  mImpl->detach_callbacks();
}

void platform::gpio::DigitalPin::disconnect() const
{
  // do nothing ?
  // TODO issue #132
}

} // namespace gpio
} // namespace platform
} // namespace lampda

#include <map>

#include "simulator_state.h"

#include "src/system/platform/gpio.h"

#include "src/system/utils/input_output.h"
#include "src/system/utils/utils.h"

#include "src/system/physical/indicator.h"

#include "simulator/include/hardware_influencer.h"

#define PLATFORM_GPIO_CPP

typedef void (*voidFuncPtr)(void);

namespace mock_gpios {

inline static std::map<DigitalPin::GPIO, voidFuncPtr> callbacks;
static bool isButtonPressed = false;
void update_callbacks()
{
  static bool wasButtonPressed = false;
  for (const auto& [pin, callback]: callbacks)
  {
    if (pin == buttonPin)
    {
      isButtonPressed = sim::globals::state.isButtonPressed;
      if (isButtonPressed != wasButtonPressed)
      {
        callback();
        wasButtonPressed = isButtonPressed;
      }
    }
  }
}

} // namespace mock_gpios

namespace mock_indicator {
COLOR idColor;
uint32_t get_color() { return idColor.color; }
} // namespace mock_indicator

namespace mock_battery {
float voltage;
} // namespace mock_battery

class DigitalPinImpl
{
public:
  DigitalPinImpl(DigitalPin::GPIO pin) : _pin(pin) {}
  ~DigitalPinImpl() = default;

  bool is_high()
  {
    // button pin
    if (_pin == buttonPin)
    {
      return !mock_gpios::isButtonPressed;
    }

    return false;
  }

  void write(uint16_t value)
  {
    switch (_pin)
    {
      case RedIndicator:
        {
          mock_indicator::idColor.red = (value & 255) / indicator::redColorCorrection;
          break;
        }
      case GreenIndicator:
        {
          // correction value for the real luminosity
          mock_indicator::idColor.green = (value & 255) / indicator::greenColorCorrection;
          break;
        }
      case BlueIndicator:
        {
          mock_indicator::idColor.blue = (value & 255) / indicator::blueColorCorrection;
          break;
        }
    }
  }

  uint16_t read()
  {
    switch (_pin)
    {
    }
    return 0;
  }

  void attach_callback(voidFuncPtr cllbk) { mock_gpios::callbacks[_pin] = cllbk; }

  void detach_callbacks() { mock_gpios::callbacks.erase(_pin); }

public:
  DigitalPin::GPIO _pin;
};

DigitalPin::DigitalPin(DigitalPin::GPIO pin) : mGpio(pin) { mImpl = std::make_shared<DigitalPinImpl>(pin); }
DigitalPin::~DigitalPin() = default;
DigitalPin::DigitalPin(const DigitalPin& other) = default;

DigitalPin& DigitalPin::operator=(const DigitalPin& other) = default;

void DigitalPin::set_pin_mode(Mode mode) {}

bool DigitalPin::is_high() const { return mImpl->is_high(); }

void DigitalPin::set_high(bool is_high) {}

void DigitalPin::write(uint16_t value) { mImpl->write(value); }

uint16_t DigitalPin::read() const { return mImpl->read(); }

int DigitalPin::pin() const { return 0; }

void DigitalPin::attach_callback(voidFuncPtr func, Interrupt mode)
{
  // TODO issue #132: handle different interrupts
  DigitalPin::s_gpiosWithInterrupts.emplace(mGpio);
  mImpl->attach_callback(func);
}

void DigitalPin::detach_callbacks()
{
  DigitalPin::s_gpiosWithInterrupts.erase(mGpio);
  mImpl->detach_callbacks();
}

void DigitalPin::disconnect()
{
  // do nothing ?
  // TODO issue #132
}

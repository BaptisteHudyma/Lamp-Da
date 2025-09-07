#include <map>

#include "simulator_state.h"

#include "src/system/platform/gpio.h"
#include "src/system/platform/print.h"

#include "src/system/utils/input_output.h"
#include "src/system/utils/utils.h"

#include "src/system/physical/indicator.h"

#include "simulator/include/hardware_influencer.h"

#define PLATFORM_GPIO_CPP

typedef void (*voidFuncPtr)(void);

namespace mock_gpios {

inline static std::map<DigitalPin::GPIO, voidFuncPtr> callbacksRisingEdge;
inline static std::map<DigitalPin::GPIO, voidFuncPtr> callbacksFallingEdge;
inline static std::map<DigitalPin::GPIO, voidFuncPtr> callbacksChange;

static bool isButtonPressed = false;

// only update button :/ thats bad
void update_callbacks()
{
  static bool wasButtonPressed = false;

  isButtonPressed = sim::globals::state.isButtonPressed;

  // event on change
  if (isButtonPressed != wasButtonPressed)
  {
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

  void attach_callback(voidFuncPtr cllbk, DigitalPin::Interrupt mode)
  {
    // cannot have two interrupt callback types
    mock_gpios::callbacksChange.erase(_pin);
    mock_gpios::callbacksRisingEdge.erase(_pin);
    mock_gpios::callbacksFallingEdge.erase(_pin);

    switch (mode)
    {
      case DigitalPin::Interrupt::kFallingEdge:
        mock_gpios::callbacksFallingEdge[_pin] = cllbk;
        break;

      case DigitalPin::Interrupt::kRisingEdge:
        mock_gpios::callbacksRisingEdge[_pin] = cllbk;
        break;

      case DigitalPin::Interrupt::kChange:
        mock_gpios::callbacksChange[_pin] = cllbk;
        break;

      default:
        break;
    }
  }

  void detach_callbacks()
  {
    mock_gpios::callbacksChange.erase(_pin);
    mock_gpios::callbacksRisingEdge.erase(_pin);
    mock_gpios::callbacksFallingEdge.erase(_pin);
  }

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
  DigitalPin::s_gpiosWithInterrupts.emplace(mGpio);
  mImpl->attach_callback(func, mode);
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

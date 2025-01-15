#include "src/system/platform/gpio.h"

#include "src/system/utils/input_output.h"
#include <SFML/Graphics.hpp>

#include "simulator/include/hardware_influencer.h"

#define PLATFORM_GPIO_CPP

typedef void (*voidFuncPtr)(void);

namespace mock_gpios {

static std::map<DigitalPin::GPIO, voidFuncPtr> callbacks;
static bool isButtonPressed = false;
void update_callbacks()
{
  for (const auto& [pin, callback]: callbacks)
  {
    if (pin == buttonPin)
    {
      if (mock_registers::isDeepSleep)
      {
        // wake up on space press
        mock_registers::isDeepSleep = not sf::Keyboard::isKeyPressed(sf::Keyboard::Space);
        continue;
      }

      isButtonPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Space);
      if (isButtonPressed)
        callback();
    }
  }
}

} // namespace mock_gpios

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
  uint16_t read() { return 0; }

  void attach_callback(voidFuncPtr cllbk) { mock_gpios::callbacks[_pin] = cllbk; }

public:
  DigitalPin::GPIO _pin;
};

DigitalPin::DigitalPin(DigitalPin::GPIO pin) { mImpl = std::make_shared<DigitalPinImpl>(pin); }
DigitalPin::~DigitalPin() = default;
DigitalPin::DigitalPin(const DigitalPin& other) = default;

DigitalPin& DigitalPin::operator=(const DigitalPin& other) = default;

void DigitalPin::set_pin_mode(Mode mode) {}

bool DigitalPin::is_high() const { return mImpl->is_high(); }

void DigitalPin::set_high(bool is_high) {}

void DigitalPin::write(uint16_t value) {}

uint16_t DigitalPin::read() const { return mImpl->read(); }

int DigitalPin::pin() const { return 0; }

void DigitalPin::attach_callback(voidFuncPtr func, Interrupt mode) { mImpl->attach_callback(func); }

void brigthness_write_analog(uint16_t value) {}
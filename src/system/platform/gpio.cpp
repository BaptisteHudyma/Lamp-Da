#ifndef PLATFORM_GPIO_CPP
#define PLATFORM_GPIO_CPP

#include "gpio.h"

#include <Arduino.h>
#include <memory>

class DigitalPinImpl
{
public:
  DigitalPinImpl(int pin) : mDigitalPin(pin) {}
  ~DigitalPinImpl() = default;

  void set_pin_mode(DigitalPin::Mode mode)
  {
    switch (mode)
    {
      case DigitalPin::kInput:
        pinMode(mDigitalPin, INPUT);
        break;
      case DigitalPin::kOutput:
        pinMode(mDigitalPin, OUTPUT);
        break;
      case DigitalPin::kInputPullUp:
        pinMode(mDigitalPin, INPUT_PULLUP_SENSE);
        break;
      case DigitalPin::kOutputHighCurrent:
        pinMode(mDigitalPin, OUTPUT_H0H1);
        break;
    }
  }
  bool is_high() { return HIGH == digitalRead(mDigitalPin); }
  void set_high(bool value) { digitalWrite(mDigitalPin, value ? HIGH : LOW); }

  uint16_t read() { return analogRead(mDigitalPin); }
  void write(uint16_t value) { analogWrite(mDigitalPin, value); }

  void attach_callback(voidFuncPtr func, DigitalPin::Interrupt mode)
  {
    const auto pinInterr = digitalPinToInterrupt(mDigitalPin);
    switch (mode)
    {
      case DigitalPin::kChange:
        attachInterrupt(pinInterr, func, CHANGE);
        break;
      case DigitalPin::kRisingEdge:
        attachInterrupt(pinInterr, func, RISING);
        break;
      case DigitalPin::kFallingEdge:
        attachInterrupt(pinInterr, func, FALLING);
        break;
      default:
        break;
    }
  }

  int mDigitalPin;
};

DigitalPin::DigitalPin(GPIO pin)
{
  switch (pin)
  {
    case GPIO::a0:
      mImpl = std::make_shared<DigitalPinImpl>(AD0);
      break;
    case GPIO::a1:
      mImpl = std::make_shared<DigitalPinImpl>(AD1);
      break;
    case GPIO::a2:
      mImpl = std::make_shared<DigitalPinImpl>(AD2);
      break;
    case GPIO::p4:
      mImpl = std::make_shared<DigitalPinImpl>(D4);
      break;
    case GPIO::p5:
      mImpl = std::make_shared<DigitalPinImpl>(D5);
      break;
    case GPIO::p6:
      mImpl = std::make_shared<DigitalPinImpl>(D6);
      break;
    case GPIO::p7:
      mImpl = std::make_shared<DigitalPinImpl>(D7);
      break;
    case GPIO::p8:
      mImpl = std::make_shared<DigitalPinImpl>(D8);
      break;
    case GPIO::ChargerInterrupt:
      mImpl = std::make_shared<DigitalPinImpl>(CHARGE_INT);
      break;
    case GPIO::ImuPower:
      mImpl = std::make_shared<DigitalPinImpl>(PIN_LSM6DS3TR_C_POWER);
      break;
    case GPIO::otgSignal:
      mImpl = std::make_shared<DigitalPinImpl>(ENABLE_OTG);
      break;
    case GPIO::usb33Power:
      mImpl = std::make_shared<DigitalPinImpl>(USB_33V_PWR);
      break;
    case GPIO::microphonePower:
      mImpl = std::make_shared<DigitalPinImpl>(PIN_PDM_PWR);
      break;
    case GPIO::chargerOkSignal:
      mImpl = std::make_shared<DigitalPinImpl>(CHARGE_OK);
      break;
  }
}
DigitalPin::~DigitalPin() = default;
DigitalPin::DigitalPin(const DigitalPin& other) = default;

DigitalPin& DigitalPin::operator=(const DigitalPin& other) = default;

void DigitalPin::set_pin_mode(Mode mode) { mImpl->set_pin_mode(mode); }

bool DigitalPin::is_high() const { return mImpl->is_high(); }

void DigitalPin::set_high(bool isHigh) { mImpl->set_high(isHigh); }

void DigitalPin::write(uint16_t value) { mImpl->write(value); }

uint16_t DigitalPin::read() const { return mImpl->read(); }

int DigitalPin::pin() const { return mImpl->mDigitalPin; }

void DigitalPin::attach_callback(voidFuncPtr func, Interrupt mode) { mImpl->attach_callback(func, mode); }

extern void brigthness_write_analog(uint16_t value) { analogWrite(OUT_BRIGHTNESS, value); }

#endif
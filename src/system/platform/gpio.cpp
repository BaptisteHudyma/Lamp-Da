#ifndef PLATFORM_GPIO_CPP
#define PLATFORM_GPIO_CPP

#include "gpio.h"

#include <Arduino.h>
#include <memory>

#include "src/system/utils/constants.h"

// check expected firmware version
#ifndef LAMPDA_FIRMWARE_VERSION_MAJOR
#error "Undefined firmware version major"
#endif

#ifndef LAMPDA_FIRMWARE_VERSION_MINOR
#error "Undefined firmware version minor"
#endif

// check expected firmware version
#ifndef EXPECTED_FIRMWARE_VERSION_MAJOR
#error "Undefined expected firmware version major"
#endif

#ifndef EXPECTED_FIRMWARE_VERSION_MINOR
#error "Undefined expected firmware version minor"
#endif

#if EXPECTED_FIRMWARE_VERSION_MAJOR != LAMPDA_FIRMWARE_VERSION_MAJOR || \
        EXPECTED_FIRMWARE_VERSION_MINOR != LAMPDA_FIRMWARE_VERSION_MINOR
#error "Firmware version missmatch"
#endif

class DigitalPinImpl
{
public:
  DigitalPinImpl(int pin) : mDigitalPin(pin) {}
  ~DigitalPinImpl() = default;

  void set_pin_mode(DigitalPin::Mode mode)
  {
    switch (mode)
    {
      case DigitalPin::kDefault:
        // trust the system, the pin mode is already set
        break;
      case DigitalPin::kInput:
        pinMode(mDigitalPin, INPUT);
        break;
      case DigitalPin::kOutput:
        pinMode(mDigitalPin, OUTPUT);
        break;
      case DigitalPin::kInputPullUp:
        pinMode(mDigitalPin, INPUT_PULLUP);
        break;
      case DigitalPin::kInputPullUpSense:
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

  void detach_callbacks()
  {
    const auto pinInterr = digitalPinToInterrupt(mDigitalPin);
    detachInterrupt(pinInterr);
  }

  int mDigitalPin;
};

DigitalPin::DigitalPin(GPIO pin) : mGpio(pin)
{
  switch (pin)
  {
    case GPIO::gpio0:
      mImpl = std::make_shared<DigitalPinImpl>(D0);
      break;
    case GPIO::gpio1:
      mImpl = std::make_shared<DigitalPinImpl>(D1);
      break;
    case GPIO::gpio2:
      mImpl = std::make_shared<DigitalPinImpl>(D2);
      break;
    case GPIO::gpio3:
      mImpl = std::make_shared<DigitalPinImpl>(D3);
      break;
    case GPIO::gpio4:
      mImpl = std::make_shared<DigitalPinImpl>(D4);
      break;
    case GPIO::gpio5:
      mImpl = std::make_shared<DigitalPinImpl>(D5);
      break;
    case GPIO::gpio6:
      mImpl = std::make_shared<DigitalPinImpl>(D6);
      break;
    case GPIO::gpio7:
      mImpl = std::make_shared<DigitalPinImpl>(D7);
      break;

    case GPIO::Input_isChargeOk:
      mImpl = std::make_shared<DigitalPinImpl>(I_IS_CHARGE_OK);
      break;

    case GPIO::Signal_PowerDelivery:
      mImpl = std::make_shared<DigitalPinImpl>(I_INT_PD_SIGNAL);
      break;
    case GPIO::Signal_UsbProtectionFault:
      mImpl = std::make_shared<DigitalPinImpl>(I_INT_USB_PROT_FAULT);
      break;
    case GPIO::Signal_VbusGateFault:
      mImpl = std::make_shared<DigitalPinImpl>(I_INT_VBUS_GATE_FAULT);
      break;
    case GPIO::Signal_ChargerProcHot:
      mImpl = std::make_shared<DigitalPinImpl>(I_INT_CHARGE_PROC_HOT);
      break;
    case GPIO::Signal_BatteryBalancerAlert:
      mImpl = std::make_shared<DigitalPinImpl>(I_INT_BLNC_ALERT);
      break;
    case GPIO::Signal_ImuInterrupt1:
      mImpl = std::make_shared<DigitalPinImpl>(I_INT_IMU_INT1);
      break;
    case GPIO::Signal_ImuInterrupt2:
      mImpl = std::make_shared<DigitalPinImpl>(I_INT_IMU_INT2);
      break;

    case GPIO::Output_EnableExternalPeripherals:
      mImpl = std::make_shared<DigitalPinImpl>(O_EN_EXT_PWR);
      break;
    case GPIO::Output_EnableMicrophone:
      mImpl = std::make_shared<DigitalPinImpl>(O_EN_PDM_PWR);
      break;
    case GPIO::Output_VbusFastRoleSwap:
      mImpl = std::make_shared<DigitalPinImpl>(O_VBUS_FRS);
      break;
    case GPIO::Output_VbusDirection:
      mImpl = std::make_shared<DigitalPinImpl>(O_VBUS_DIR);
      break;
    case GPIO::Output_EnableOnTheGo:
      mImpl = std::make_shared<DigitalPinImpl>(O_ENABLE_OTG);
      break;

    case GPIO::Output_DischargeVbus:
      mImpl = std::make_shared<DigitalPinImpl>(O_VBUS_DISCHARGE);
      break;
    case GPIO::Output_EnableVbusGate:
      mImpl = std::make_shared<DigitalPinImpl>(O_EN_VBUS_GATE);
      break;
    case GPIO::Output_EnableOutputGate:
      mImpl = std::make_shared<DigitalPinImpl>(O_EN_OUTPUT_PWR);
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

#endif
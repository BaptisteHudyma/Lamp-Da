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
#error "Firmware version missmatch, please update the base repository"
#endif

// Register function to disconnect gpios
void disconnect_pin(uint32_t ulPin)
{
  if (ulPin >= PINS_COUNT)
  {
    return;
  }
  nrf_gpio_cfg_default(g_ADigitalPinMap[ulPin]);
}

class DigitalPinImpl
{
public:
  DigitalPinImpl(int pin) : mDigitalPin(pin) {}
  ~DigitalPinImpl() = default;

  void set_pin_mode(DigitalPin::Mode mode) const
  {
    switch (mode)
    {
      case DigitalPin::Mode::kDefault:
        // trust the system, the pin mode is already set
        break;
      case DigitalPin::Mode::kInput:
        pinMode(mDigitalPin, INPUT);
        break;
      case DigitalPin::Mode::kOutput:
        pinMode(mDigitalPin, OUTPUT);
        // prevent brief flash at startup
        set_high(false);
        break;
      case DigitalPin::Mode::kInputPullUp:
        pinMode(mDigitalPin, INPUT_PULLUP);
        break;
      case DigitalPin::Mode::kInputPullUpSense:
        pinMode(mDigitalPin, INPUT_PULLUP_SENSE);
        break;
      case DigitalPin::Mode::kOutputHighCurrent:
        pinMode(mDigitalPin, OUTPUT_H0H1);
        break;
    }
  }
  bool is_high() const { return HIGH == digitalRead(mDigitalPin); }
  void set_high(bool value) const { digitalWrite(mDigitalPin, value ? HIGH : LOW); }

  uint16_t read() const { return analogRead(mDigitalPin); }
  void write(uint16_t value) const { analogWrite(mDigitalPin, value); }

  void attach_callback(DigitalPin::voidFuncPtr func, DigitalPin::Interrupt mode) const
  {
    const auto pinInterr = digitalPinToInterrupt(mDigitalPin);
    switch (mode)
    {
      case DigitalPin::Interrupt::kChange:
        attachInterrupt(pinInterr, func, CHANGE);
        break;
      case DigitalPin::Interrupt::kRisingEdge:
        attachInterrupt(pinInterr, func, RISING);
        break;
      case DigitalPin::Interrupt::kFallingEdge:
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

void DigitalPin::set_pin_mode(Mode mode) const { mImpl->set_pin_mode(mode); }

bool DigitalPin::is_high() const { return mImpl->is_high(); }

void DigitalPin::set_high(bool isHigh) const { mImpl->set_high(isHigh); }

void DigitalPin::write(uint16_t value) const { mImpl->write(value); }

uint16_t DigitalPin::read() const { return mImpl->read(); }

int DigitalPin::pin() const { return mImpl->mDigitalPin; }

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

void DigitalPin::disconnect() const { disconnect_pin(mImpl->mDigitalPin); }

#endif

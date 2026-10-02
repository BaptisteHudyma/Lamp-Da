#include "src/system/hal/gpio.h"

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
#error "Firmware version missmatch, please update the base lampda_nrf52840 repository using the command 'make safe-install' "
#endif

namespace lampda {
namespace hal {
namespace gpio {

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

DigitalPin::DigitalPin(GPIO pin) : mGpio(pin) { set(pin); }

inline static DigitalPinImpl gpio_objects[] = {DigitalPinImpl(D0),
                                               DigitalPinImpl(D1),
                                               DigitalPinImpl(D2),
                                               DigitalPinImpl(D3),
                                               DigitalPinImpl(D4),
                                               DigitalPinImpl(D5),
                                               DigitalPinImpl(D6),
                                               DigitalPinImpl(D7),
                                               DigitalPinImpl(I_IS_CHARGE_OK),
                                               DigitalPinImpl(I_INT_PD_SIGNAL),
                                               DigitalPinImpl(I_INT_USB_PROT_FAULT),
                                               DigitalPinImpl(I_INT_VBUS_GATE_FAULT),
                                               DigitalPinImpl(I_INT_CHARGE_PROC_HOT),
                                               DigitalPinImpl(I_INT_BLNC_ALERT),
                                               DigitalPinImpl(I_INT_IMU_INT1),
                                               DigitalPinImpl(I_INT_IMU_INT2),
                                               DigitalPinImpl(O_EN_EXT_PWR),
                                               DigitalPinImpl(O_EN_PDM_PWR),
                                               DigitalPinImpl(O_VBUS_FRS),
                                               DigitalPinImpl(O_VBUS_DIR),
                                               DigitalPinImpl(O_ENABLE_OTG),
                                               DigitalPinImpl(O_VBUS_DISCHARGE),
                                               DigitalPinImpl(O_EN_VBUS_GATE),
                                               DigitalPinImpl(O_EN_OUTPUT_PWR)};

int gpio_to_index(DigitalPin::GPIO pin)
{
  switch (pin)
  {
    case DigitalPin::GPIO::gpio0:
      return 0;
    case DigitalPin::GPIO::gpio1:
      return 1;
    case DigitalPin::GPIO::gpio2:
      return 2;
    case DigitalPin::GPIO::gpio3:
      return 3;
    case DigitalPin::GPIO::gpio4:
      return 4;
    case DigitalPin::GPIO::gpio5:
      return 5;
    case DigitalPin::GPIO::gpio6:
      return 6;
    case DigitalPin::GPIO::gpio7:
      return 7;
    case DigitalPin::GPIO::Input_isChargeOk:
      return 8;
    case DigitalPin::GPIO::Signal_PowerDelivery:
      return 9;
    case DigitalPin::GPIO::Signal_UsbProtectionFault:
      return 10;
    case DigitalPin::GPIO::Signal_VbusGateFault:
      return 11;
    case DigitalPin::GPIO::Signal_ChargerProcHot:
      return 12;
    case DigitalPin::GPIO::Signal_BatteryBalancerAlert:
      return 13;
    case DigitalPin::GPIO::Signal_ImuInterrupt1:
      return 14;
    case DigitalPin::GPIO::Signal_ImuInterrupt2:
      return 15;
    case DigitalPin::GPIO::Output_EnableExternalPeripherals:
      return 16;
    case DigitalPin::GPIO::Output_EnableMicrophone:
      return 17;
    case DigitalPin::GPIO::Output_VbusFastRoleSwap:
      return 18;
    case DigitalPin::GPIO::Output_VbusDirection:
      return 19;
    case DigitalPin::GPIO::Output_EnableOnTheGo:
      return 20;
    case DigitalPin::GPIO::Output_DischargeVbus:
      return 21;
    case DigitalPin::GPIO::Output_EnableVbusGate:
      return 22;
    case DigitalPin::GPIO::Output_EnableOutputGate:
      return 23;
  }
  return -1;
}

void DigitalPin::set(DigitalPin::GPIO pin)
{
  mGpio = pin;

  const int gpioIndex = gpio_to_index(pin);
  if (gpioIndex >= 0)
    mImpl = {std::shared_ptr<DigitalPinImpl> {}, &gpio_objects[gpioIndex]};
  else
    mImpl = nullptr;
}

void DigitalPin::set_pin_mode(Mode mode) const
{
  if (mImpl)
    mImpl->set_pin_mode(mode);
}

bool DigitalPin::is_high() const { return mImpl and mImpl->is_high(); }

void DigitalPin::set_high(bool isHigh) const
{
  if (mImpl)
    mImpl->set_high(isHigh);
}

void DigitalPin::write(uint16_t value) const
{
  if (mImpl)
    mImpl->write(value);
}

uint16_t DigitalPin::read() const
{
  if (mImpl)
    return mImpl->read();
  return 0;
}

int DigitalPin::pin() const
{
  if (mImpl)
    return mImpl->mDigitalPin;
  return 0;
}

void DigitalPin::attach_callback(voidFuncPtr func, Interrupt mode) const
{
  if (mImpl)
  {
    DigitalPin::s_gpiosWithInterrupts.set(static_cast<uint16_t>(mGpio));
    mImpl->attach_callback(func, mode);
  }
}

void DigitalPin::detach_callbacks() const
{
  if (mImpl)
  {
    DigitalPin::s_gpiosWithInterrupts.reset(static_cast<uint16_t>(mGpio));
    mImpl->detach_callbacks();
  }
}

void DigitalPin::disconnect() const
{
  if (mImpl)
    disconnect_pin(mImpl->mDigitalPin);
}

} // namespace gpio
} // namespace hal
} // namespace lampda

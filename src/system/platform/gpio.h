#ifndef PLATFORM_GPIO_H
#define PLATFORM_GPIO_H

#include <set>
#include <memory>
#include <stdint.h>

class DigitalPinImpl;

class DigitalPin
{
public:
  enum class GPIO
  {
    gpio0,
    gpio1,
    gpio2,
    gpio3,
    gpio4,
    gpio5,
    gpio6,
    gpio7,

    Input_isChargeOk,

    Signal_PowerDelivery,
    Signal_UsbProtectionFault,
    Signal_VbusGateFault,
    Signal_ChargerProcHot,
    Signal_BatteryBalancerAlert,
    Signal_ImuInterrupt1,
    Signal_ImuInterrupt2,

    Output_EnableExternalPeripherals,
    Output_EnableMicrophone,
    Output_VbusFastRoleSwap,
    Output_VbusDirection,
    Output_EnableOnTheGo,
    // danger zone: only one of the next 3 signals should be active at a time
    Output_DischargeVbus,
    Output_EnableVbusGate,
    Output_EnableOutputGate
  };
  enum class Mode
  {
    kDefault = 0,
    kInput,
    kOutput,

    kInputPullUp,
    kInputPullUpSense,
    kOutputHighCurrent,
  };
  enum class Interrupt
  {
    kChange,
    kRisingEdge,
    kFallingEdge,
  };

  DigitalPin(GPIO pin);

  DigitalPin(DigitalPin&& other) = delete;

  void set_pin_mode(Mode mode) const;
  bool is_high() const; // true if high, false if low
  void set_high(bool isHigh) const;

  void write(uint16_t value) const;
  uint16_t read() const;

  int pin() const; // get the raw pin id

  using voidFuncPtr = void (*)(void);
  void attach_callback(voidFuncPtr func, Interrupt mode) const;
  void detach_callbacks() const;

  // if called, this pin will be physically disconnected from any power/gnd
  // if will need to be reinitialized
  void disconnect() const;

  static void detach_all()
  {
    // detach all set interrupts
    for (DigitalPin::GPIO pin: DigitalPin::s_gpiosWithInterrupts)
    {
      DigitalPin(pin).detach_callbacks();
    }
    DigitalPin::s_gpiosWithInterrupts.clear();
  }

  // call this when the gpios needs to be deactivated
  static void deactivate_gpios()
  {
    // loop though all gpios, deactivate them all
    for (uint32_t pin = static_cast<uint32_t>(GPIO::gpio0); pin != static_cast<uint32_t>(GPIO::Output_EnableOutputGate);
         ++pin)
    {
      DigitalPin((GPIO)pin).disconnect();
    }
  }

private:
  inline static std::set<GPIO> s_gpiosWithInterrupts;

  GPIO mGpio;
  std::shared_ptr<DigitalPinImpl> mImpl;
};

#endif

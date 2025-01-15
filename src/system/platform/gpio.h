#ifndef PLATFORM_GPIO_H
#define PLATFORM_GPIO_H

#include <memory>
#include <stdint.h>

class DigitalPinImpl;

class DigitalPin
{
public:
  enum GPIO
  {
    a0,
    a1,
    a2,
    p4,
    p5,
    p6,
    p7,
    p8,

    ChargerInterrupt,
    ImuPower,
    otgSignal,
    usb33Power,
    microphonePower,
    chargerOkSignal,
    batterySignal,
  };
  enum Mode
  {
    kInput = 0,
    kOutput,

    kInputPullUp,
    kOutputHighCurrent,
  };
  enum Interrupt
  {
    kChange,
    kRisingEdge,
    kFallingEdge,
  };

  DigitalPin(GPIO pin);
  ~DigitalPin();
  DigitalPin(const DigitalPin& other);
  DigitalPin& operator=(const DigitalPin& other);

  DigitalPin(DigitalPin&& other) = delete;

  void set_pin_mode(Mode mode);
  bool is_high() const; // true if high, false if low
  void set_high(bool isHigh);

  void write(uint16_t value);
  uint16_t read() const;

  int pin() const; // get the raw pin id

  typedef void (*voidFuncPtr)(void);
  void attach_callback(voidFuncPtr func, Interrupt mode);

private:
  std::shared_ptr<DigitalPinImpl> mImpl;
};

extern void brigthness_write_analog(uint16_t value);

#endif
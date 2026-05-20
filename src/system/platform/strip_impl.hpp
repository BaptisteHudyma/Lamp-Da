/**
 * \file strip_impl.hpp
 * \brief Hardware interface for a strip of LEDs. This is a simple implementation that uses the Arduino library.
 * It assumes the device support EasyDMA (NRF52)
 */

#ifndef PLATFORM_STRIPIMPL_HPP
#define PLATFORM_STRIPIMPL_HPP

#include "strip_impl.h"

#include "src/system/platform/print.h"

#include <Arduino.h>
#include <cassert>

namespace lampda {
namespace platform {
/// Define the interaction layer with an indexable strip
namespace strip {

template<size_t LedCount, uint8_t ChannelCount> bool LampdaStrip<LedCount, ChannelCount>::begin(void)
{
  if (pin >= 0)
  {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
  }
  else
  {
    begun = false;
    return false;
  }
  begun = true;
  return true;
}

template<size_t LedCount, uint8_t ChannelCount>
void LampdaStrip<LedCount, ChannelCount>::setPixelColor(uint16_t n, uint32_t c)
{
  if (n < numLEDs)
  {
    uint8_t *p, r = (uint8_t)(c >> 16), g = (uint8_t)(c >> 8), b = (uint8_t)c;
    if (wOffset == rOffset)
    {
      p = &pixels[n * 3];
    }
    else
    {
      p = &pixels[n * 4];
      uint8_t w = (uint8_t)(c >> 24);
      p[wOffset] = w;
    }
    p[rOffset] = r;
    p[gOffset] = g;
    p[bOffset] = b;
  }
}

template<size_t LedCount, uint8_t ChannelCount>
uint32_t LampdaStrip<LedCount, ChannelCount>::getPixelColor(uint16_t n) const
{
  if (n >= numLEDs)
    return 0; // Out of bounds, return no color.

  if (wOffset == rOffset)
  { // Is RGB-type device
    uint8_t const* p = &pixels[n * 3];
    // No b constrightness adjustment has been made -- return 'raw' color
    return ((uint32_t)p[rOffset] << 16) | ((uint32_t)p[gOffset] << 8) | (uint32_t)p[bOffset];
  }
  else
  { // Is RGBW-type device
    uint8_t const* p = &pixels[n * 4];
    return ((uint32_t)p[wOffset] << 24) | ((uint32_t)p[rOffset] << 16) | ((uint32_t)p[gOffset] << 8) |
           (uint32_t)p[bOffset];
  }
}

template<size_t LedCount, uint8_t ChannelCount> bool LampdaStrip<LedCount, ChannelCount>::canShow(void)
{
  // It's normal and possible for endTime to exceed micros() if the
  // 32-bit clock counter has rolled over (about every 70 minutes).
  // Since both are uint32_t, a negative delta correctly maps back to
  // positive space, and it would seem like the subtraction below would
  // suffice. But a problem arises if code invokes show() very
  // infrequently...the micros() counter may roll over MULTIPLE times in
  // that interval, the delta calculation is no longer correct and the
  // next update may stall for a very long time. The check below resets
  // the latch counter if a rollover has occurred. This can cause an
  // extra delay of up to 300 microseconds in the rare case where a
  // show() call happens precisely around the rollover, but that's
  // neither likely nor especially harmful, vs. other code that might
  // stall for 30+ minutes, or having to document and frequently remind
  // and/or provide tech support explaining an unintuitive need for
  // show() calls at least once an hour.
  uint32_t now = micros();
  if (endTime > now)
  {
    endTime = now;
  }
  return (now - endTime) >= 300L;
}

template<size_t LedCount, uint8_t ChannelCount> void LampdaStrip<LedCount, ChannelCount>::updateType(neoPixelType t)
{
  wOffset = (t >> 6) & 0b11; // See notes in header file
  rOffset = (t >> 4) & 0b11; // regarding R/G/B/W offsets
  gOffset = (t >> 2) & 0b11;
  bOffset = t & 0b11;

  if (wOffset == rOffset)
  {
    assert(ChannelCount == 3);
  }
  else
  {
    assert(ChannelCount == 4);
  }
}

template<size_t LedCount, uint8_t ChannelCount> void LampdaStrip<LedCount, ChannelCount>::setPin(int16_t p)
{
  if (begun && (pin >= 0))
    pinMode(pin, INPUT); // Disable existing out pin
  pin = p;
  if (begun)
  {
    pinMode(p, OUTPUT);
    digitalWrite(p, LOW);
  }
}

template<size_t LedCount, uint8_t ChannelCount> void LampdaStrip<LedCount, ChannelCount>::show(void)
{
  // clang-format off

  // Data latch = 300+ microsecond pause in the output stream. Rather than
  // put a delay at the end of the function, the ending time is noted and
  // the function will simply hold off (if needed) on issuing the
  // subsequent round of data until the latch time has elapsed. This
  // allows the mainline code to start generating the next frame of data
  // rather than stalling for the latch.
  while (!canShow())
    ;

  // endTime is a private member (rather than global var) so that multiple
  // instances on different pins can be quickly issued in succession (each
  // instance doesn't delay the next).


// [[[Begin of the Neopixel NRF52 EasyDMA implementation
//                                    by the Hackerspace San Salvador]]]
// This technique uses the PWM peripheral on the NRF52. The PWM uses the
// EasyDMA feature included on the chip. This technique loads the duty
// cycle configuration for each cycle when the PWM is enabled. For this
// to work we need to store a 16 bit configuration for each bit of the
// RGB(W) values in the pixel buffer.
// Comparator values for the PWM were hand picked and are guaranteed to
// be 100% organic to preserve freshness and high accuracy. Current
// parameters are:
//   * PWM Clock: 16Mhz
//   * Minimum step time: 62.5ns
//   * Time for zero in high (T0H): 0.31ms
//   * Time for one in high (T1H): 0.75ms
//   * Cycle time:  1.25us
//   * Frequency: 800Khz
// For 400Khz we just double the calculated times.
// ---------- BEGIN Constants for the EasyDMA implementation -----------
// The PWM starts the duty cycle in LOW. To start with HIGH we
// need to set the 15th bit on each register.

// WS2812 (rev A) timing is 0.35 and 0.7us
// #define MAGIC_T0H               5UL | (0x8000) // 0.3125us
// #define MAGIC_T1H              12UL | (0x8000) // 0.75us

// WS2812B (rev B) timing is 0.4 and 0.8 us
#define MAGIC_T0H 6UL | (0x8000)  // 0.375us
#define MAGIC_T1H 13UL | (0x8000) // 0.8125us

// WS2811 (400 khz) timing is 0.5 and 1.2
#define MAGIC_T0H_400KHz 8UL | (0x8000)  // 0.5us
#define MAGIC_T1H_400KHz 19UL | (0x8000) // 1.1875us

// For 400Khz, we double value of CTOPVAL
#define CTOPVAL        20UL // 1.25us
#define CTOPVAL_400KHz 40UL // 2.5us

// ---------- END Constants for the EasyDMA implementation -------------

  // To support both the SoftDevice + Neopixels we use the EasyDMA
  // feature from the NRF25. However this technique implies to
  // generate a pattern and store it on the memory. The actual
  // memory used in bytes corresponds to the following formula:
  //              totalMem = numBytes*8*2+(2*2)
  // The two additional bytes at the end are needed to reset the
  // sequence.

  NRF_PWM_Type* pwm = NULL;

  // Try to find a free PWM device, which is not enabled
  // and has no connected pins
  NRF_PWM_Type* PWM[] = {NRF_PWM0,
                         NRF_PWM1,
                         NRF_PWM2
#if defined(NRF_PWM3)
                         ,
                         NRF_PWM3
#endif
  };

  for (unsigned int device = 0; device < (sizeof(PWM) / sizeof(PWM[0])); device++)
  {
    if ((PWM[device]->ENABLE == 0) && (PWM[device]->PSEL.OUT[0] & PWM_PSEL_OUT_CONNECT_Msk) &&
        (PWM[device]->PSEL.OUT[1] & PWM_PSEL_OUT_CONNECT_Msk) &&
        (PWM[device]->PSEL.OUT[2] & PWM_PSEL_OUT_CONNECT_Msk) && (PWM[device]->PSEL.OUT[3] & PWM_PSEL_OUT_CONNECT_Msk))
    {
      pwm = PWM[device];
      break;
    }
  }

  // Use the identified device to choose the implementation
  // If a PWM device is available use DMA
  if ((pixels_pattern != NULL) && (pwm != NULL))
  {
    uint16_t pos = 0; // bit position

    for (uint16_t n = 0; n < numBytes; n++)
    {
      uint8_t pix = pixels[n];

      for (uint8_t mask = 0x80; mask > 0; mask >>= 1)
      {
#if defined(NEO_KHZ400)
        if (!is800KHz)
        {
          pixels_pattern[pos] = (pix & mask) ? MAGIC_T1H_400KHz : MAGIC_T0H_400KHz;
        }
        else
#endif
        {
          pixels_pattern[pos] = (pix & mask) ? MAGIC_T1H : MAGIC_T0H;
        }

        pos++;
      }
    }

    // Zero padding to indicate the end of que sequence
    pixels_pattern[pos++] = 0 | (0x8000); // Seq end
    pixels_pattern[pos++] = 0 | (0x8000); // Seq end

    // Set the wave mode to count UP
    pwm->MODE = (PWM_MODE_UPDOWN_Up << PWM_MODE_UPDOWN_Pos);

    // Set the PWM to use the 16MHz clock
    pwm->PRESCALER = (PWM_PRESCALER_PRESCALER_DIV_1 << PWM_PRESCALER_PRESCALER_Pos);

    // Setting of the maximum count
    // but keeping it on 16Mhz allows for more granularity just
    // in case someone wants to do more fine-tuning of the timing.
#if defined(NEO_KHZ400)
    if (!is800KHz)
    {
      pwm->COUNTERTOP = (CTOPVAL_400KHz << PWM_COUNTERTOP_COUNTERTOP_Pos);
    }
    else
#endif
    {
      pwm->COUNTERTOP = (CTOPVAL << PWM_COUNTERTOP_COUNTERTOP_Pos);
    }

    // Disable loops, we want the sequence to repeat only once
    pwm->LOOP = (PWM_LOOP_CNT_Disabled << PWM_LOOP_CNT_Pos);

    // On the "Common" setting the PWM uses the same pattern for the
    // for supported sequences. The pattern is stored on half-word
    // of 16bits
    pwm->DECODER =
            (PWM_DECODER_LOAD_Common << PWM_DECODER_LOAD_Pos) | (PWM_DECODER_MODE_RefreshCount << PWM_DECODER_MODE_Pos);

    // Pointer to the memory storing the patter
    pwm->SEQ[0].PTR = (uint32_t)(pixels_pattern) << PWM_SEQ_PTR_PTR_Pos;

    // Calculation of the number of steps loaded from memory.
    pwm->SEQ[0].CNT = pattern_size << PWM_SEQ_CNT_CNT_Pos;

    // The following settings are ignored with the current config.
    pwm->SEQ[0].REFRESH = 0;
    pwm->SEQ[0].ENDDELAY = 0;

    // The Neopixel implementation is a blocking algorithm. DMA
    // allows for non-blocking operation. To "simulate" a blocking
    // operation we enable the interruption for the end of sequence
    // and block the execution thread until the event flag is set by
    // the peripheral.
    //    pwm->INTEN |= (PWM_INTEN_SEQEND0_Enabled<<PWM_INTEN_SEQEND0_Pos);

// PSEL must be configured before enabling PWM
#if defined(ARDUINO_ARCH_NRF52840)
    pwm->PSEL.OUT[0] = g_APinDescription[pin].name;
#else
    pwm->PSEL.OUT[0] = g_ADigitalPinMap[pin];
#endif

    // Enable the PWM
    pwm->ENABLE = 1;

    // After all of this and many hours of reading the documentation
    // we are ready to start the sequence...
    pwm->EVENTS_SEQEND[0] = 0;
    pwm->TASKS_SEQSTART[0] = 1;

    // But we have to wait for the flag to be set.
    while (!pwm->EVENTS_SEQEND[0])
    {
#if defined(ARDUINO_NRF52_ADAFRUIT) || defined(ARDUINO_ARCH_NRF52840)
      yield();
#endif
    }

    // Before leave we clear the flag for the event.
    pwm->EVENTS_SEQEND[0] = 0;

    // We need to disable the device and disconnect
    // all the outputs before leave or the device will not
    // be selected on the next call.
    // TODO: Check if disabling the device causes performance issues.
    pwm->ENABLE = 0;

    pwm->PSEL.OUT[0] = 0xFFFFFFFFUL;
  } // End of DMA implementation
  // ---------------------------------------------------------------------
  else
  {
    /// THIS SHOULD NEVER HAPPEN.
    // If this case appears, it means that a register has gone very wrong.
    platform::lampda_print("WRONG EXECUTION PATH FOR LAMPDA STRIP DISPLAY");
  }
  // END of NRF52 implementation

  endTime = micros(); // Save EOD time for latch on next call

  // clang-format on
}

} // namespace strip
} // namespace platform
} // namespace lampda

#endif

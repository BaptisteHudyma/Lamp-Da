/**
 * \file strip_impl.cpp
 * \brief Hardware interface for a strip of LEDs. This is a simple implementation that uses the Arduino library.
 * It assumes the device support EasyDMA (NRF52)
 */

#ifndef PLATFORM_STRIPIMPL_CPP
#define PLATFORM_STRIPIMPL_CPP

#include "strip_impl.h"

#include "src/system/platform/print.h"
#include "src/system/platform/time.h"

#include "src/user/constants.h"

#include <Arduino.h>
#include <cassert>

static constexpr uint8_t DMABufferIndex = 0;
static constexpr uint32_t SHOW_TIMEOUT_MS = 300;

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
    uint8_t* p;
    uint8_t r = (uint8_t)(c >> 16);
    uint8_t g = (uint8_t)(c >> 8);
    uint8_t b = (uint8_t)c;
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

  if constexpr (ChannelCount == 3)
  {
    assert(wOffset == rOffset);
  }
  else if constexpr (ChannelCount == 4)
  {
    assert(wOffset != rOffset);
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

/*
 * \brief Interrupt event to handle the end of a DMA writter event.
 * This function is called when the DMA has finished writing to a buffer.
 *
 * \param pwmId The PWM interface ID that triggered the event.
 * \param pwm The PWM peripheral instance.
 */
template<size_t LedCount, uint8_t ChannelCount>
void LampdaStrip<LedCount, ChannelCount>::___irq_dma_update(const uint8_t pwmId, void* voidPwm, const bool forceUpdate)
{
  if (_selectedPwmInterface != pwmId)
    return;
  NRF_PWM_Type* pwm = (NRF_PWM_Type*)voidPwm;
  if (pwm->EVENTS_SEQEND[DMABufferIndex] or forceUpdate)
  {
    // THIS FLAG MUST BE LOWERED OR THE INTERRUPT WILL BE CALLED AGAIN
    pwm->EVENTS_SEQEND[DMABufferIndex] = 0;
    // AND THE SYSTEM WILL CRASH

    // set end flag if needed
    _isFinished = _writtenByteCnt >= _expectedBitWritten;
    if (_isFinished)
      return;

    // set the new buffer: swap with old one
    pwm->SEQ[DMABufferIndex].PTR = (uint32_t)(_dataBuffers[_bufferToWriteIndex]) << PWM_SEQ_PTR_PTR_Pos;
    // start sequence again
    pwm->TASKS_SEQSTART[DMABufferIndex] = 1;
    _writtenByteCnt += _dataBufferSize;

    // if not finished, fill the other buffer
    // This oscillates between 0 and 1
    _bufferToWriteIndex = (_bufferToWriteIndex + 1) & 1;
    this->write_data_buffers(_writtenLedChannelCnt, _dataBuffers[_bufferToWriteIndex]);

    _evt_cnt++;
  }
}

template<size_t LedCount, uint8_t ChannelCount>
void LampdaStrip<LedCount, ChannelCount>::write_data_buffers(const uint16_t startIndex, uint16_t* dataBuffer)
{
// WS2812 (rev A) timing is 0.35 and 0.7us
// #define MAGIC_T0H               5UL | (0x8000) // 0.3125us
// #define MAGIC_T1H              12UL | (0x8000) // 0.75us

// WS2812B (rev B) timing is 0.4 and 0.8 us
#define MAGIC_T0H 6UL | 0x8000  // 0.375us
#define MAGIC_T1H 13UL | 0x8000 // 0.8125us

  uint16_t pos = 0; // bit position
  const uint16_t endOfLoop =
          min<uint16_t>(LampdaStrip<LedCount, ChannelCount>::numBytes, startIndex + _ledChannelPerBufferCount);
  // fill the data with the input data
  for (uint16_t n = startIndex; n < endOfLoop; n++)
  {
    uint8_t pix = this->pixels[n];
    for (uint8_t mask = 0x80; mask > 0; mask >>= 1)
    {
      dataBuffer[pos] = (pix & mask) ? MAGIC_T1H : MAGIC_T0H;
      pos++;
    }
    _writtenLedChannelCnt += 1;
  }
  // fill the remainder, if it exist
  uint8_t endBits = 0;
  for (uint16_t n = pos; n < _dataBufferSize; n++)
  {
    endBits++;
    dataBuffer[n] = 0 | 0x8000; // Seq end
  }
  _call_cnt++;
}

template<size_t LedCount, uint8_t ChannelCount> void LampdaStrip<LedCount, ChannelCount>::show(void)
{
  // clang-format on

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

  // ---------- END Constants for the EasyDMA implementation -------------

  // To support both the SoftDevice + Neopixels we use the EasyDMA
  // feature from the NRF25. However this technique implies to
  // generate a pattern and store it on the memory. The actual
  // memory used in bytes corresponds to the following formula:
  //              totalMem = numBytes*8*2+(2*2)
  // The two additional bytes at the end are needed to reset the
  // sequence.

  NRF_PWM_Type* pwm = nullptr;

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

  IRQn_Type irqTypes[] = {PWM0_IRQn,
                          PWM1_IRQn,
                          PWM2_IRQn,
#if defined(NRF_PWM3)
                          PWM3_IRQn
#endif
  };

  uint8_t pwmIndex = 0;
  for (uint8_t device = 0; device < (sizeof(PWM) / sizeof(PWM[0])); device++)
  {
    if ((PWM[device]->ENABLE == 0) && (PWM[device]->PSEL.OUT[0] & PWM_PSEL_OUT_CONNECT_Msk) &&
        (PWM[device]->PSEL.OUT[1] & PWM_PSEL_OUT_CONNECT_Msk) &&
        (PWM[device]->PSEL.OUT[2] & PWM_PSEL_OUT_CONNECT_Msk) && (PWM[device]->PSEL.OUT[3] & PWM_PSEL_OUT_CONNECT_Msk))
    {
      pwmIndex = device;
      pwm = PWM[device];
      break;
    }
  }

  // Use the identified device to choose the implementation
  // If a PWM device is available use DMA
  if (pwm != nullptr)
  {
    NVIC_DisableIRQ(irqTypes[pwmIndex]);

    // For 400Khz, we double value of CTOPVAL
    static constexpr uint16_t CTOPVAL = 20UL; // 1.25us

    nrf_pwm_configure(pwm,
                      // Set the PWM to use the 16MHz clock
                      nrf_pwm_clk_t::NRF_PWM_CLK_16MHz,
                      // Set the wave mode to count UP
                      nrf_pwm_mode_t::NRF_PWM_MODE_UP,
                      // Setting of the maximum count
                      // but keeping it on 16Mhz allows for more granularity just
                      // in case someone wants to do more fine-tuning of the timing.
                      CTOPVAL);

    // Disable loops, we want the sequence to repeat only once
    nrf_pwm_loop_set(pwm, PWM_LOOP_CNT_Disabled << PWM_LOOP_CNT_Pos);
    // On the "Common" setting the PWM uses the same pattern for the
    // for supported sequences. The pattern is stored on half-word of 16bits
    nrf_pwm_decoder_set(pwm, nrf_pwm_dec_load_t::NRF_PWM_LOAD_COMMON, nrf_pwm_dec_step_t::NRF_PWM_STEP_AUTO);

#if defined(ARDUINO_ARCH_NRF52840)
    const auto pinOut = g_APinDescription[pin].name;
#else
    const auto pinOut = g_ADigitalPinMap[pin];
#endif
    pwm->PSEL.OUT[0] = pinOut;

    // The following settings are ignored with the current config.
    nrf_pwm_seq_refresh_set(pwm, DMABufferIndex, 0);
    nrf_pwm_seq_end_delay_set(pwm, DMABufferIndex, 0);

    // enable the driver
    pwm->ENABLE = 1;

    NVIC_ClearPendingIRQ(irqTypes[pwmIndex]);
    NVIC_SetPriority(irqTypes[pwmIndex], configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY);
    nrf_pwm_int_enable(pwm, (DMABufferIndex == 0 ? PWM_INTENSET_SEQEND0_Msk : PWM_INTENSET_SEQEND1_Msk));

    // set interrupts
    NVIC_EnableIRQ(irqTypes[pwmIndex]);

    // prepare first run
    _bufferToWriteIndex = 0;
    _isFinished = false;
    _writtenByteCnt = 0;
    _writtenLedChannelCnt = 0;
    _selectedPwmInterface = pwmIndex;
    // buffer size is fixed
    pwm->SEQ[DMABufferIndex].CNT = _dataBufferSize << PWM_SEQ_CNT_CNT_Pos;
    // first write data into the first buffer
    this->write_data_buffers(_writtenLedChannelCnt, _dataBuffers[_bufferToWriteIndex]);
    // quickstart the cycle
    ___irq_dma_update(pwmIndex, pwm, true);

    // This loop waits for the write and interrupt routine to end
    const uint32_t startTime = platform::time_ms();
    while (not _isFinished and platform::time_ms() - startTime < SHOW_TIMEOUT_MS)
    {
#if defined(ARDUINO_NRF52_ADAFRUIT) || defined(ARDUINO_ARCH_NRF52840)
      yield();
#endif
    }
    _isFinished = true;

    // Before leave we clear the flag for the event.
    nrf_pwm_int_disable(pwm, PWM_INTENSET_SEQEND0_Msk | PWM_INTENSET_SEQEND1_Msk);
    pwm->TASKS_STOP = PWM_TASKS_STOP_TASKS_STOP_Msk;
    platform::delay_ms(1);
    NVIC_DisableIRQ(irqTypes[pwmIndex]);

    if (platform::time_ms() - startTime > SHOW_TIMEOUT_MS)
      lampda::platform::lampda_print("Show event timeout !");
    if (_writtenByteCnt < _expectedBitWritten)
      lampda::platform::lampda_print("Written event missmatch: events %d, write calls %d, bit written %d/%d",
                                     _evt_cnt,
                                     _call_cnt,
                                     _writtenByteCnt,
                                     _expectedBitWritten);

    _evt_cnt = 0;
    _call_cnt = 0;
    _writtenByteCnt = 0;
    _writtenLedChannelCnt = 0;

    // We need to disable the device and disconnect
    // all the outputs before leave or the device will not
    // be selected on the next call.
    // TODO: Check if disabling the device causes performance issues.
    nrf_pwm_disable(pwm);
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

// Template instanciation to use a .cpp file
template class LampdaStrip<LED_COUNT, 3>;

// Define the strip object
LampdaStrip<::lampda::LED_COUNT, 3> stripHardwareObject;

} // namespace strip
} // namespace platform
} // namespace lampda

extern "C" {
  // Static linking to the event interrupts
  void PWM0_IRQHandler(void) { lampda::platform::strip::stripHardwareObject.___irq_dma_update(0, NRF_PWM0); }
  void PWM1_IRQHandler(void) { lampda::platform::strip::stripHardwareObject.___irq_dma_update(1, NRF_PWM1); }
  void PWM2_IRQHandler(void) { lampda::platform::strip::stripHardwareObject.___irq_dma_update(2, NRF_PWM2); }
#ifdef NRF_PWM3
  void PWM3_IRQHandler(void) { lampda::platform::strip::stripHardwareObject.___irq_dma_update(3, NRF_PWM3); }
#endif
}

#endif

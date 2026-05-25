/*! \file strip_impl_mock.cpp
    \brief Mock of the indexable strip library
*/

#ifndef PLATFORM_STRIPIMPL_CPP
#define PLATFORM_STRIPIMPL_CPP

#include "src/system/platform/strip_impl.h"
#include "src/system/platform/time.h"

#include "src/system/utils/colorspace.h"
#include "src/system/utils/utils.h"
#include "src/user/constants.h"

#include <memory>
#include <cassert>

namespace lampda {
namespace platform {
/// Define the interaction layer with an indexable strip
namespace strip {

template<size_t LedCount, uint8_t ChannelCount> bool LampdaStrip<LedCount, ChannelCount>::begin(void)
{
  if (pin < 0)
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
  uint32_t now = platform::time_us();
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

template<size_t LedCount, uint8_t ChannelCount> void LampdaStrip<LedCount, ChannelCount>::setPin(int16_t p) {}

template<size_t LedCount, uint8_t ChannelCount> void LampdaStrip<LedCount, ChannelCount>::show(void)
{
  /// Cannot implement here, simulator handles this
}

#ifdef LMBD_LAMP_TYPE__INDEXABLE
// Template instanciation to use a .cpp file
template class LampdaStrip<LED_COUNT, 3>;

// Define the strip object
LampdaStrip<::lampda::LED_COUNT, 3> stripHardwareObject;
#endif

} // namespace strip
} // namespace platform
} // namespace lampda

#endif

#include <cstdint>
#include <limits>
#include <gtest/gtest.h>

#include "src/modes/include/colors/utils.hpp"
#include "src/modes/include/colors/palettes.hpp"

namespace lampda::modes::colors {

TEST(test_colors, color_blend16)
{
  uint32_t c1 = fromRGB(0, 0, 0);
  uint32_t c2 = fromRGB(255, 255, 255);
  ASSERT_EQ(blend<uint16_t>(c1, c2, 0), fromRGB(0, 0, 0));
  ASSERT_EQ(blend<uint16_t>(c1, c2, 16384), fromRGB(63, 63, 63));
  ASSERT_EQ(blend<uint16_t>(c1, c2, 32768), fromRGB(127, 127, 127));
  ASSERT_EQ(blend<uint16_t>(c1, c2, 49152), fromRGB(191, 191, 191));
  ASSERT_EQ(blend<uint16_t>(c1, c2, UINT16_MAX), fromRGB(255, 255, 255));

  ASSERT_EQ(blend<uint16_t>(c2, c1, 0), fromRGB(255, 255, 255));
  ASSERT_EQ(blend<uint16_t>(c2, c1, 16384), fromRGB(191, 191, 191));
  ASSERT_EQ(blend<uint16_t>(c2, c1, 32768), fromRGB(127, 127, 127));
  ASSERT_EQ(blend<uint16_t>(c2, c1, 49152), fromRGB(63, 63, 63));
  ASSERT_EQ(blend<uint16_t>(c2, c1, UINT16_MAX), fromRGB(0, 0, 0));
}

TEST(test_colors, color_blend8)
{
  uint32_t c1 = fromRGB(0, 0, 0);
  uint32_t c2 = fromRGB(255, 255, 255);
  ASSERT_EQ(blend<uint8_t>(c1, c2, 0), fromRGB(0, 0, 0));
  ASSERT_EQ(blend<uint8_t>(c1, c2, 64), fromRGB(64, 64, 64));
  ASSERT_EQ(blend<uint8_t>(c1, c2, 128), fromRGB(128, 128, 128));
  ASSERT_EQ(blend<uint8_t>(c1, c2, 192), fromRGB(192, 192, 192));
  ASSERT_EQ(blend<uint8_t>(c1, c2, UINT8_MAX), fromRGB(255, 255, 255));

  ASSERT_EQ(blend<uint8_t>(c2, c1, 0), fromRGB(255, 255, 255));
  ASSERT_EQ(blend<uint8_t>(c2, c1, 64), fromRGB(191, 191, 191));
  ASSERT_EQ(blend<uint8_t>(c2, c1, 128), fromRGB(127, 127, 127));
  ASSERT_EQ(blend<uint8_t>(c2, c1, 192), fromRGB(63, 63, 63));
  ASSERT_EQ(blend<uint8_t>(c2, c1, UINT8_MAX), fromRGB(0, 0, 0));
}

/**
 * Test palettes
 */

TEST(test_palette, b8_red_palette)
{
  for (uint8_t redChanel = 0; redChanel < UINT8_MAX; redChanel++)
  {
    const uint32_t color = fromRGB(redChanel, 0, 0);

    PaletteTy palette;
    palette.fill(color);

    for (uint8_t i = 0; i < UINT8_MAX; i++)
    {
      ASSERT_EQ(from_palette<false>(i, palette), color);
      ASSERT_EQ(from_palette<true>(i, palette), color);
    }
  }
}

TEST(test_palette, b8_green_palette)
{
  for (uint8_t greenChanel = 0; greenChanel < UINT8_MAX; greenChanel++)
  {
    const uint32_t color = fromRGB(0, greenChanel, 0);
    PaletteTy palette;
    palette.fill(color);

    for (uint8_t i = 0; i < UINT8_MAX; i++)
    {
      ASSERT_EQ(from_palette<false>(i, palette), color);
      ASSERT_EQ(from_palette<true>(i, palette), color);
    }
  }
}

TEST(test_palette, b8_blue_palette)
{
  for (uint8_t blueChanel = 0; blueChanel < UINT8_MAX; blueChanel++)
  {
    const uint32_t color = fromRGB(0, 0, blueChanel);
    PaletteTy palette;
    palette.fill(color);

    for (uint8_t i = 0; i < UINT8_MAX; i++)
    {
      ASSERT_EQ(from_palette<false>(i, palette), color);
      ASSERT_EQ(from_palette<true>(i, palette), color);
    }
  }
}

TEST(test_palette, b8_white_palette)
{
  for (uint8_t chanel = 0; chanel < UINT8_MAX; chanel++)
  {
    const uint32_t color = fromRGB(chanel, chanel, chanel);
    PaletteTy palette;
    palette.fill(color);

    for (uint8_t i = 0; i < UINT8_MAX; i++)
    {
      ASSERT_EQ(from_palette<false>(i, palette), color);
      ASSERT_EQ(from_palette<true>(i, palette), color);
    }
  }
}

TEST(test_palette, b8_brightness)
{
  const uint32_t color = fromRGB(255, 255, 255);
  PaletteTy palette;
  palette.fill(color);

  // check brightness
  for (uint8_t i = 0; i < UINT8_MAX; i++)
  {
    // check all indices
    for (uint8_t j = 0; j < UINT8_MAX; j++)
    {
      ASSERT_EQ(from_palette<false>(j, palette, i), fromRGB(i, i, i));
      ASSERT_EQ(from_palette<true>(j, palette, i), fromRGB(i, i, i));
    }
  }
}

TEST(test_palette, b16_red_palette)
{
  for (uint8_t redChanel = 0; redChanel < UINT8_MAX; redChanel++)
  {
    const uint32_t color = redChanel << 16;

    PaletteTy palette;
    palette.fill(color);

    for (uint16_t i = 0; i < UINT16_MAX; i++)
    {
      ASSERT_EQ(from_palette<false>(i, palette), color);
      ASSERT_EQ(from_palette<true>(i, palette), color);
    }
  }
}

TEST(test_palette, b16_green_palette)
{
  for (uint8_t greenChanel = 0; greenChanel < UINT8_MAX; greenChanel++)
  {
    const uint32_t color = greenChanel << 8;
    PaletteTy palette;
    palette.fill(color);

    for (uint16_t i = 0; i < UINT16_MAX; i++)
    {
      ASSERT_EQ(from_palette<false>(i, palette), color);
      ASSERT_EQ(from_palette<true>(i, palette), color);
    }
  }
}

TEST(test_palette, b16_blue_palette)
{
  for (uint8_t blueChanel = 0; blueChanel < UINT8_MAX; blueChanel++)
  {
    const uint32_t color = blueChanel;
    PaletteTy palette;
    palette.fill(color);

    for (uint16_t i = 0; i < UINT16_MAX; i++)
    {
      ASSERT_EQ(from_palette<false>(i, palette), color);
      ASSERT_EQ(from_palette<true>(i, palette), color);
    }
  }
}

TEST(test_palette, b16_white_palette)
{
  for (uint8_t chanel = 0; chanel < UINT8_MAX; chanel++)
  {
    const uint32_t color = chanel << 16 | chanel << 8 | chanel;
    PaletteTy palette;
    palette.fill(color);

    for (uint16_t i = 0; i < UINT16_MAX; i++)
    {
      ASSERT_EQ(from_palette<false>(i, palette), color);
      ASSERT_EQ(from_palette<true>(i, palette), color);
    }
  }
}

TEST(test_palette, b16_brightness)
{
  const uint32_t color = 0xFFFFFF;
  PaletteTy palette;
  palette.fill(color);

  // check brightness
  for (uint8_t i = 0; i < UINT8_MAX; i++)
  {
    // check all indices
    for (uint16_t j = 0; j < UINT16_MAX; j++)
    {
      ASSERT_EQ(from_palette<false>(j, palette, i), i << 16 | i << 8 | i << 0);
      ASSERT_EQ(from_palette<true>(j, palette, i), i << 16 | i << 8 | i << 0);
    }
  }
}

} // namespace lampda::modes::colors

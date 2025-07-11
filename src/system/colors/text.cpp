#ifdef LMBD_LAMP_TYPE__INDEXABLE

#include <sys/types.h>

#include <cstdint>
#include <string>

#include "text.h"

#include "src/system/utils/constants.h"
#include "src/system/utils/utils.h"
#include "src/system/utils/print.h"

namespace text {

// fonts from:
// http://www.piclist.com/techref/datafile/charset/extractor/charset_extractor.html

struct IFont
{
  virtual const uint8_t* get_letter(const u_char letter) const = 0;

  virtual uint8_t get_height() const = 0;
  virtual uint8_t get_width() const = 0;

  inline int8_t get_arrayLenght() const { return get_height() * get_width() / 8; };
};

struct SmallFont : public IFont
{
  static inline constexpr uint8_t font[96][6] = {
          {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, //
          {0x10, 0xE3, 0x84, 0x10, 0x01, 0x00}, // !
          {0x6D, 0xB4, 0x80, 0x00, 0x00, 0x00}, // "
          {0x00, 0xA7, 0xCA, 0x29, 0xF2, 0x80}, // #
          {0x20, 0xE4, 0x0C, 0x09, 0xC1, 0x00}, // $
          {0x65, 0x90, 0x84, 0x21, 0x34, 0xC0}, // %
          {0x21, 0x45, 0x08, 0x55, 0x23, 0x40}, // &
          {0x30, 0xC2, 0x00, 0x00, 0x00, 0x00}, // '
          {0x10, 0x82, 0x08, 0x20, 0x81, 0x00}, // (
          {0x20, 0x41, 0x04, 0x10, 0x42, 0x00}, // )
          {0x00, 0xA3, 0x9F, 0x38, 0xA0, 0x00}, // *
          {0x00, 0x41, 0x1F, 0x10, 0x40, 0x00}, // +
          {0x00, 0x00, 0x00, 0x00, 0xC3, 0x08}, // ,
          {0x00, 0x00, 0x1F, 0x00, 0x00, 0x00}, // -
          {0x00, 0x00, 0x00, 0x00, 0xC3, 0x00}, // .
          {0x00, 0x10, 0x84, 0x21, 0x00, 0x00}, // /
          {0x39, 0x14, 0xD5, 0x65, 0x13, 0x80}, // 0
          {0x10, 0xC1, 0x04, 0x10, 0x43, 0x80}, // 1
          {0x39, 0x10, 0x46, 0x21, 0x07, 0xC0}, // 2
          {0x39, 0x10, 0x4E, 0x05, 0x13, 0x80}, // 3
          {0x08, 0x62, 0x92, 0x7C, 0x20, 0x80}, // 4
          {0x7D, 0x04, 0x1E, 0x05, 0x13, 0x80}, // 5
          {0x18, 0x84, 0x1E, 0x45, 0x13, 0x80}, // 6
          {0x7C, 0x10, 0x84, 0x20, 0x82, 0x00}, // 7
          {0x39, 0x14, 0x4E, 0x45, 0x13, 0x80}, // 8
          {0x39, 0x14, 0x4F, 0x04, 0x23, 0x00}, // 9
          {0x00, 0x03, 0x0C, 0x00, 0xC3, 0x00}, // :
          {0x00, 0x03, 0x0C, 0x00, 0xC3, 0x08}, // ;
          {0x08, 0x42, 0x10, 0x20, 0x40, 0x80}, // <
          {0x00, 0x07, 0xC0, 0x01, 0xF0, 0x00}, // =
          {0x20, 0x40, 0x81, 0x08, 0x42, 0x00}, // >
          {0x39, 0x10, 0x46, 0x10, 0x01, 0x00}, // ?
          {0x39, 0x15, 0xD5, 0x5D, 0x03, 0x80}, // @
          {0x39, 0x14, 0x51, 0x7D, 0x14, 0x40}, // A
          {0x79, 0x14, 0x5E, 0x45, 0x17, 0x80}, // B
          {0x39, 0x14, 0x10, 0x41, 0x13, 0x80}, // C
          {0x79, 0x14, 0x51, 0x45, 0x17, 0x80}, // D
          {0x7D, 0x04, 0x1E, 0x41, 0x07, 0xC0}, // E
          {0x7D, 0x04, 0x1E, 0x41, 0x04, 0x00}, // F
          {0x39, 0x14, 0x17, 0x45, 0x13, 0xC0}, // G
          {0x45, 0x14, 0x5F, 0x45, 0x14, 0x40}, // H
          {0x38, 0x41, 0x04, 0x10, 0x43, 0x80}, // I
          {0x04, 0x10, 0x41, 0x45, 0x13, 0x80}, // J
          {0x45, 0x25, 0x18, 0x51, 0x24, 0x40}, // K
          {0x41, 0x04, 0x10, 0x41, 0x07, 0xC0}, // L
          {0x45, 0xB5, 0x51, 0x45, 0x14, 0x40}, // M
          {0x45, 0x95, 0x53, 0x45, 0x14, 0x40}, // N
          {0x39, 0x14, 0x51, 0x45, 0x13, 0x80}, // O
          {0x79, 0x14, 0x5E, 0x41, 0x04, 0x00}, // P
          {0x39, 0x14, 0x51, 0x55, 0x23, 0x40}, // Q
          {0x79, 0x14, 0x5E, 0x49, 0x14, 0x40}, // R
          {0x39, 0x14, 0x0E, 0x05, 0x13, 0x80}, // S
          {0x7C, 0x41, 0x04, 0x10, 0x41, 0x00}, // T
          {0x45, 0x14, 0x51, 0x45, 0x13, 0x80}, // U
          {0x45, 0x14, 0x51, 0x44, 0xA1, 0x00}, // V
          {0x45, 0x15, 0x55, 0x55, 0x52, 0x80}, // W
          {0x45, 0x12, 0x84, 0x29, 0x14, 0x40}, // X
          {0x45, 0x14, 0x4A, 0x10, 0x41, 0x00}, // Y
          {0x78, 0x21, 0x08, 0x41, 0x07, 0x80}, // Z
          {0x38, 0x82, 0x08, 0x20, 0x83, 0x80}, // [
          {0x01, 0x02, 0x04, 0x08, 0x10, 0x00}, // /
          {0x38, 0x20, 0x82, 0x08, 0x23, 0x80}, // ]
          {0x10, 0xA4, 0x40, 0x00, 0x00, 0x00}, // ^
          {0x00, 0x00, 0x00, 0x00, 0x00, 0x3F}, // _
          {0x30, 0xC1, 0x00, 0x00, 0x00, 0x00}, // `
          {0x00, 0x03, 0x81, 0x3D, 0x13, 0xC0}, // a
          {0x41, 0x07, 0x91, 0x45, 0x17, 0x80}, // b
          {0x00, 0x03, 0x91, 0x41, 0x13, 0x80}, // c
          {0x04, 0x13, 0xD1, 0x45, 0x13, 0xC0}, // d
          {0x00, 0x03, 0x91, 0x79, 0x03, 0x80}, // e
          {0x18, 0x82, 0x1E, 0x20, 0x82, 0x00}, // f
          {0x00, 0x03, 0xD1, 0x44, 0xF0, 0x4E}, // g
          {0x41, 0x07, 0x12, 0x49, 0x24, 0x80}, // h
          {0x10, 0x01, 0x04, 0x10, 0x41, 0x80}, // i
          {0x08, 0x01, 0x82, 0x08, 0x24, 0x8C}, // j
          {0x41, 0x04, 0x94, 0x61, 0x44, 0x80}, // k
          {0x10, 0x41, 0x04, 0x10, 0x41, 0x80}, // l
          {0x00, 0x06, 0x95, 0x55, 0x14, 0x40}, // m
          {0x00, 0x07, 0x12, 0x49, 0x24, 0x80}, // n
          {0x00, 0x03, 0x91, 0x45, 0x13, 0x80}, // o
          {0x00, 0x07, 0x91, 0x45, 0x17, 0x90}, // p
          {0x00, 0x03, 0xD1, 0x45, 0x13, 0xC1}, // q
          {0x00, 0x05, 0x89, 0x20, 0x87, 0x00}, // r
          {0x00, 0x03, 0x90, 0x38, 0x13, 0x80}, // s
          {0x00, 0x87, 0x88, 0x20, 0xA1, 0x00}, // t
          {0x00, 0x04, 0x92, 0x49, 0x62, 0x80}, // u
          {0x00, 0x04, 0x51, 0x44, 0xA1, 0x00}, // v
          {0x00, 0x04, 0x51, 0x55, 0xF2, 0x80}, // w
          {0x00, 0x04, 0x92, 0x31, 0x24, 0x80}, // x
          {0x00, 0x04, 0x92, 0x48, 0xE1, 0x18}, // y
          {0x00, 0x07, 0x82, 0x31, 0x07, 0x80}, // z
          {0x18, 0x82, 0x18, 0x20, 0x81, 0x80}, // {
          {0x10, 0x41, 0x00, 0x10, 0x41, 0x00}, // |
          {0x30, 0x20, 0x83, 0x08, 0x23, 0x00}, // }
          {0x29, 0x40, 0x00, 0x00, 0x00, 0x00}, // ~
          {0x10, 0xE6, 0xD1, 0x45, 0xF0, 0x00}  // 
  };

  const uint8_t* get_letter(const u_char letter) const override
  {
    if (letter >= 32 and letter < 128)
      // array starts at ascii 32
      return SmallFont::font[letter - 32];
    return SmallFont::font[1]; // return the char corresponding to '!'
  }

  inline uint8_t get_height() const override { return 8; };
  inline uint8_t get_width() const override { return 6; };
};
static const SmallFont smallFont;

struct BigFont : public IFont
{
  static inline constexpr uint8_t font[96][24] = {
          {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, //
          {0x06, 0x00, 0x60, 0x0F, 0x00, 0xF0, 0x0F, 0x00, 0xF0, 0x0F, 0x00, 0x60,
           0x06, 0x00, 0x60, 0x00, 0x00, 0x00, 0x06, 0x00, 0x60, 0x00, 0x00, 0x00}, // !
          {0x00, 0x00, 0x00, 0x19, 0x81, 0x98, 0x19, 0x81, 0x98, 0x00, 0x00, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // "
          {0x00, 0x00, 0x66, 0x06, 0x60, 0x66, 0x3F, 0xF0, 0xCC, 0x0C, 0xC1, 0x98,
           0x19, 0x87, 0xFC, 0x33, 0x03, 0x30, 0x33, 0x00, 0x00, 0x00, 0x00, 0x00}, // #
          {0x06, 0x00, 0x60, 0x1F, 0x83, 0xFC, 0x36, 0x03, 0x60, 0x3F, 0x81, 0xFC,
           0x06, 0xC0, 0x6C, 0x3F, 0xC1, 0xF8, 0x06, 0x00, 0x60, 0x00, 0x00, 0x00}, // $
          {0x00, 0x00, 0x00, 0x00, 0x13, 0x83, 0x38, 0x73, 0x8E, 0x01, 0xC0, 0x38,
           0x07, 0x00, 0xE0, 0x1C, 0x03, 0x8E, 0x70, 0xE6, 0x0E, 0x00, 0x00, 0x00}, // %
          {0x00, 0x00, 0x70, 0x0D, 0x81, 0x98, 0x19, 0x81, 0xB0, 0x0E, 0x01, 0xE0,
           0x3E, 0x03, 0x36, 0x33, 0xC3, 0x18, 0x3B, 0xC1, 0xE6, 0x00, 0x00, 0x00}, // &
          {0x0E, 0x00, 0xE0, 0x0E, 0x00, 0x60, 0x06, 0x00, 0xC0, 0x00, 0x00, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // '
          {0x03, 0x80, 0x60, 0x0E, 0x00, 0xC0, 0x1C, 0x01, 0xC0, 0x1C, 0x01, 0xC0,
           0x1C, 0x01, 0xC0, 0x0C, 0x00, 0xE0, 0x06, 0x00, 0x38, 0x00, 0x00, 0x00}, // (
          {0x1C, 0x00, 0x60, 0x07, 0x00, 0x30, 0x03, 0x80, 0x38, 0x03, 0x80, 0x38,
           0x03, 0x80, 0x38, 0x03, 0x00, 0x70, 0x06, 0x01, 0xC0, 0x00, 0x00, 0x00}, // )
          {0x00, 0x00, 0x00, 0x00, 0x03, 0x6C, 0x36, 0xC1, 0xF8, 0x0F, 0x03, 0xFC,
           0x0F, 0x01, 0xF8, 0x36, 0xC3, 0x6C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // *
          {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x60, 0x06, 0x03, 0xFC,
           0x3F, 0xC0, 0x60, 0x06, 0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // +
          {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0x0E, 0x00, 0xE0, 0x06, 0x00, 0xC0}, // ,
          {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xFC,
           0x3F, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // -
          {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0x0E, 0x00, 0xE0, 0x00, 0x00, 0x00}, // .
          {0x00, 0x00, 0x01, 0x00, 0x30, 0x07, 0x00, 0xE0, 0x1C, 0x03, 0x80, 0x70,
           0x0E, 0x01, 0xC0, 0x38, 0x07, 0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00}, // /
          {0x0F, 0x83, 0xFE, 0x30, 0x66, 0x07, 0x60, 0xF6, 0x1B, 0x63, 0x36, 0x63,
           0x6C, 0x37, 0x83, 0x70, 0x33, 0x06, 0x3F, 0xE0, 0xF8, 0x00, 0x00, 0x00}, // 0
          {0x03, 0x00, 0x70, 0x1F, 0x01, 0xF0, 0x03, 0x00, 0x30, 0x03, 0x00, 0x30,
           0x03, 0x00, 0x30, 0x03, 0x00, 0x30, 0x1F, 0xE1, 0xFE, 0x00, 0x00, 0x00}, // 1
          {0x1F, 0xC3, 0xFE, 0x70, 0x76, 0x03, 0x60, 0x70, 0x0E, 0x01, 0xC0, 0x38,
           0x07, 0x00, 0xE0, 0x1C, 0x03, 0x80, 0x7F, 0xF7, 0xFF, 0x00, 0x00, 0x00}, // 2
          {0x1F, 0xC3, 0xFE, 0x70, 0x76, 0x03, 0x00, 0x30, 0x07, 0x0F, 0xE0, 0xFC,
           0x00, 0x60, 0x03, 0x60, 0x37, 0x07, 0x3F, 0xE1, 0xFC, 0x00, 0x00, 0x00}, // 3
          {0x01, 0xC0, 0x3C, 0x07, 0xC0, 0xEC, 0x1C, 0xC3, 0x8C, 0x70, 0xC6, 0x0C,
           0x7F, 0xF7, 0xFF, 0x00, 0xC0, 0x0C, 0x00, 0xC0, 0x0C, 0x00, 0x00, 0x00}, // 4
          {0x7F, 0xF7, 0xFF, 0x60, 0x06, 0x00, 0x60, 0x07, 0xFC, 0x3F, 0xE0, 0x07,
           0x00, 0x30, 0x03, 0x60, 0x37, 0x07, 0x3F, 0xE1, 0xFC, 0x00, 0x00, 0x00}, // 5
          {0x03, 0xC0, 0x7C, 0x0E, 0x01, 0xC0, 0x38, 0x03, 0x00, 0x7F, 0xC7, 0xFE,
           0x70, 0x76, 0x03, 0x60, 0x37, 0x07, 0x3F, 0xE1, 0xFC, 0x00, 0x00, 0x00}, // 6
          {0x7F, 0xF7, 0xFF, 0x00, 0x60, 0x06, 0x00, 0xC0, 0x0C, 0x01, 0x80, 0x18,
           0x03, 0x00, 0x30, 0x06, 0x00, 0x60, 0x0C, 0x00, 0xC0, 0x00, 0x00, 0x00}, // 7
          {0x0F, 0x81, 0xFC, 0x38, 0xE3, 0x06, 0x30, 0x63, 0x8E, 0x1F, 0xC3, 0xFE,
           0x70, 0x76, 0x03, 0x60, 0x37, 0x07, 0x3F, 0xE1, 0xFC, 0x00, 0x00, 0x00}, // 8
          {0x1F, 0xC3, 0xFE, 0x70, 0x76, 0x03, 0x60, 0x37, 0x07, 0x3F, 0xF1, 0xFF,
           0x00, 0x60, 0x0E, 0x01, 0xC0, 0x38, 0x1F, 0x01, 0xE0, 0x00, 0x00, 0x00}, // 9
          {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0E, 0x00, 0xE0, 0x0E, 0x00, 0x00,
           0x00, 0x00, 0x00, 0x0E, 0x00, 0xE0, 0x0E, 0x00, 0x00, 0x00, 0x00, 0x00}, // :
          {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0E, 0x00, 0xE0, 0x0E, 0x00, 0x00,
           0x00, 0x00, 0x00, 0x0E, 0x00, 0xE0, 0x0E, 0x00, 0x60, 0x06, 0x00, 0xC0}, // ;
          {0x00, 0xC0, 0x1C, 0x03, 0x80, 0x70, 0x0E, 0x01, 0xC0, 0x38, 0x03, 0x80,
           0x1C, 0x00, 0xE0, 0x07, 0x00, 0x38, 0x01, 0xC0, 0x0C, 0x00, 0x00, 0x00}, // <
          {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xFE, 0x3F, 0xE0, 0x00,
           0x00, 0x03, 0xFE, 0x3F, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // =
          {0x30, 0x03, 0x80, 0x1C, 0x00, 0xE0, 0x07, 0x00, 0x38, 0x01, 0xC0, 0x1C,
           0x03, 0x80, 0x70, 0x0E, 0x01, 0xC0, 0x38, 0x03, 0x00, 0x00, 0x00, 0x00}, // >
          {0x1F, 0x83, 0xFC, 0x70, 0xE6, 0x06, 0x60, 0xE0, 0x1C, 0x03, 0x80, 0x70,
           0x06, 0x00, 0x60, 0x06, 0x00, 0x00, 0x06, 0x00, 0x60, 0x00, 0x00, 0x00}, // ?
          {0x1F, 0xC3, 0xFE, 0x30, 0x66, 0x7B, 0x6F, 0xB6, 0xDB, 0x6D, 0xB6, 0xDB,
           0x6D, 0xB6, 0xFE, 0x67, 0xC7, 0x00, 0x3F, 0xC0, 0xFC, 0x00, 0x00, 0x00}, // @
          {0x06, 0x00, 0x60, 0x0F, 0x00, 0xF0, 0x0F, 0x01, 0x98, 0x19, 0x81, 0x98,
           0x30, 0xC3, 0xFC, 0x3F, 0xC6, 0x06, 0x60, 0x66, 0x06, 0x00, 0x00, 0x00}, // A
          {0x7F, 0x07, 0xF8, 0x61, 0xC6, 0x0C, 0x60, 0xC6, 0x1C, 0x7F, 0x87, 0xFC,
           0x60, 0xE6, 0x06, 0x60, 0x66, 0x0E, 0x7F, 0xC7, 0xF8, 0x00, 0x00, 0x00}, // B
          {0x0F, 0x81, 0xFC, 0x38, 0xE3, 0x06, 0x60, 0x06, 0x00, 0x60, 0x06, 0x00,
           0x60, 0x06, 0x00, 0x30, 0x63, 0x8E, 0x1F, 0xC0, 0xF8, 0x00, 0x00, 0x00}, // C
          {0x7F, 0x07, 0xF8, 0x61, 0xC6, 0x0C, 0x60, 0x66, 0x06, 0x60, 0x66, 0x06,
           0x60, 0x66, 0x06, 0x60, 0xC6, 0x1C, 0x7F, 0x87, 0xF0, 0x00, 0x00, 0x00}, // D
          {0x7F, 0xE7, 0xFE, 0x60, 0x06, 0x00, 0x60, 0x06, 0x00, 0x7F, 0x87, 0xF8,
           0x60, 0x06, 0x00, 0x60, 0x06, 0x00, 0x7F, 0xE7, 0xFE, 0x00, 0x00, 0x00}, // E
          {0x7F, 0xE7, 0xFE, 0x60, 0x06, 0x00, 0x60, 0x06, 0x00, 0x7F, 0x87, 0xF8,
           0x60, 0x06, 0x00, 0x60, 0x06, 0x00, 0x60, 0x06, 0x00, 0x00, 0x00, 0x00}, // F
          {0x0F, 0xC1, 0xFE, 0x38, 0x63, 0x00, 0x60, 0x06, 0x00, 0x63, 0xE6, 0x3E,
           0x60, 0x66, 0x06, 0x30, 0x63, 0x86, 0x1F, 0xE0, 0xFE, 0x00, 0x00, 0x00}, // G
          {0x60, 0x66, 0x06, 0x60, 0x66, 0x06, 0x60, 0x66, 0x06, 0x7F, 0xE7, 0xFE,
           0x60, 0x66, 0x06, 0x60, 0x66, 0x06, 0x60, 0x66, 0x06, 0x00, 0x00, 0x00}, // H
          {0x1F, 0x81, 0xF8, 0x06, 0x00, 0x60, 0x06, 0x00, 0x60, 0x06, 0x00, 0x60,
           0x06, 0x00, 0x60, 0x06, 0x00, 0x60, 0x1F, 0x81, 0xF8, 0x00, 0x00, 0x00}, // I
          {0x00, 0x60, 0x06, 0x00, 0x60, 0x06, 0x00, 0x60, 0x06, 0x00, 0x60, 0x06,
           0x00, 0x66, 0x06, 0x60, 0x67, 0x0C, 0x3F, 0xC1, 0xF8, 0x00, 0x00, 0x00}, // J
          {0x60, 0x66, 0x0E, 0x61, 0xC6, 0x38, 0x67, 0x06, 0xE0, 0x7C, 0x07, 0xC0,
           0x6E, 0x06, 0x70, 0x63, 0x86, 0x1C, 0x60, 0xE6, 0x06, 0x00, 0x00, 0x00}, // K
          {0x60, 0x06, 0x00, 0x60, 0x06, 0x00, 0x60, 0x06, 0x00, 0x60, 0x06, 0x00,
           0x60, 0x06, 0x00, 0x60, 0x06, 0x00, 0x7F, 0xE7, 0xFE, 0x00, 0x00, 0x00}, // L
          {0x60, 0x67, 0x0E, 0x70, 0xE7, 0x9E, 0x79, 0xE6, 0xF6, 0x6F, 0x66, 0x66,
           0x66, 0x66, 0x06, 0x60, 0x66, 0x06, 0x60, 0x66, 0x06, 0x00, 0x00, 0x00}, // M
          {0x60, 0x67, 0x06, 0x70, 0x67, 0x86, 0x6C, 0x66, 0xC6, 0x66, 0x66, 0x66,
           0x63, 0x66, 0x36, 0x61, 0xE6, 0x0E, 0x60, 0xE6, 0x06, 0x00, 0x00, 0x00}, // N
          {0x0F, 0x01, 0xF8, 0x39, 0xC3, 0x0C, 0x60, 0x66, 0x06, 0x60, 0x66, 0x06,
           0x60, 0x66, 0x06, 0x30, 0xC3, 0x9C, 0x1F, 0x80, 0xF0, 0x00, 0x00, 0x00}, // O
          {0x7F, 0x87, 0xFC, 0x60, 0xE6, 0x06, 0x60, 0x66, 0x06, 0x60, 0xE7, 0xFC,
           0x7F, 0x86, 0x00, 0x60, 0x06, 0x00, 0x60, 0x06, 0x00, 0x00, 0x00, 0x00}, // P
          {0x0F, 0x01, 0xF8, 0x39, 0xC3, 0x0C, 0x60, 0x66, 0x06, 0x60, 0x66, 0x06,
           0x60, 0x66, 0x36, 0x33, 0xC3, 0x9C, 0x1F, 0xE0, 0xF6, 0x00, 0x00, 0x00}, // Q
          {0x7F, 0x87, 0xFC, 0x60, 0xE6, 0x06, 0x60, 0x66, 0x06, 0x60, 0xE7, 0xFC,
           0x7F, 0x86, 0x70, 0x63, 0x86, 0x1C, 0x60, 0xE6, 0x06, 0x00, 0x00, 0x00}, // R
          {0x1F, 0x83, 0xFC, 0x70, 0xE6, 0x06, 0x60, 0x07, 0x00, 0x3F, 0x81, 0xFC,
           0x00, 0xE0, 0x06, 0x60, 0x67, 0x0E, 0x3F, 0xC1, 0xF8, 0x00, 0x00, 0x00}, // S
          {0x3F, 0xC3, 0xFC, 0x06, 0x00, 0x60, 0x06, 0x00, 0x60, 0x06, 0x00, 0x60,
           0x06, 0x00, 0x60, 0x06, 0x00, 0x60, 0x06, 0x00, 0x60, 0x00, 0x00, 0x00}, // T
          {0x60, 0x66, 0x06, 0x60, 0x66, 0x06, 0x60, 0x66, 0x06, 0x60, 0x66, 0x06,
           0x60, 0x66, 0x06, 0x60, 0x63, 0x0C, 0x3F, 0xC1, 0xF8, 0x00, 0x00, 0x00}, // U
          {0x60, 0x66, 0x06, 0x60, 0x63, 0x0C, 0x30, 0xC3, 0x0C, 0x19, 0x81, 0x98,
           0x19, 0x80, 0xF0, 0x0F, 0x00, 0xF0, 0x06, 0x00, 0x60, 0x00, 0x00, 0x00}, // V
          {0x60, 0x66, 0x06, 0x60, 0x66, 0x06, 0x60, 0x66, 0x06, 0x60, 0x66, 0x66,
           0x66, 0x66, 0xF6, 0x79, 0xE7, 0x0E, 0x70, 0xE6, 0x06, 0x00, 0x00, 0x00}, // W
          {0x60, 0x66, 0x06, 0x30, 0xC3, 0x0C, 0x19, 0x80, 0xF0, 0x06, 0x00, 0x60,
           0x0F, 0x01, 0x98, 0x30, 0xC3, 0x0C, 0x60, 0x66, 0x06, 0x00, 0x00, 0x00}, // X
          {0x60, 0x66, 0x06, 0x30, 0xC3, 0x0C, 0x19, 0x81, 0x98, 0x0F, 0x00, 0xF0,
           0x06, 0x00, 0x60, 0x06, 0x00, 0x60, 0x06, 0x00, 0x60, 0x00, 0x00, 0x00}, // Y
          {0x7F, 0xE7, 0xFE, 0x00, 0xC0, 0x0C, 0x01, 0x80, 0x30, 0x06, 0x00, 0x60,
           0x0C, 0x01, 0x80, 0x30, 0x03, 0x00, 0x7F, 0xE7, 0xFE, 0x00, 0x00, 0x00}, // Z
          {0x1F, 0x81, 0xF8, 0x18, 0x01, 0x80, 0x18, 0x01, 0x80, 0x18, 0x01, 0x80,
           0x18, 0x01, 0x80, 0x18, 0x01, 0x80, 0x1F, 0x81, 0xF8, 0x00, 0x00, 0x00}, // [
          {0x00, 0x04, 0x00, 0x60, 0x07, 0x00, 0x38, 0x01, 0xC0, 0x0E, 0x00, 0x70,
           0x03, 0x80, 0x1C, 0x00, 0xE0, 0x07, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00}, // /
          {0x1F, 0x81, 0xF8, 0x01, 0x80, 0x18, 0x01, 0x80, 0x18, 0x01, 0x80, 0x18,
           0x01, 0x80, 0x18, 0x01, 0x80, 0x18, 0x1F, 0x81, 0xF8, 0x00, 0x00, 0x00}, // ]
          {0x02, 0x00, 0x70, 0x0F, 0x81, 0xDC, 0x38, 0xE7, 0x07, 0x60, 0x30, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // ^
          {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7F, 0xF7, 0xFF}, // _
          {0x00, 0x00, 0x70, 0x07, 0x00, 0x70, 0x06, 0x00, 0x60, 0x03, 0x00, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // `
          {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xFC, 0x3F, 0xE0, 0x06,
           0x1F, 0xE3, 0xFE, 0x60, 0x66, 0x06, 0x7F, 0xE3, 0xFE, 0x00, 0x00, 0x00}, // a
          {0x60, 0x06, 0x00, 0x60, 0x06, 0x00, 0x60, 0x06, 0xF8, 0x7F, 0xC7, 0x0E,
           0x60, 0x66, 0x06, 0x60, 0x66, 0x0E, 0x7F, 0xC7, 0xF8, 0x00, 0x00, 0x00}, // b
          {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xF8, 0x3F, 0xC7, 0x06,
           0x60, 0x06, 0x00, 0x60, 0x07, 0x06, 0x3F, 0xC1, 0xF8, 0x00, 0x00, 0x00}, // c
          {0x00, 0x60, 0x06, 0x00, 0x60, 0x06, 0x00, 0x61, 0xF6, 0x3F, 0xE7, 0x1E,
           0x60, 0x66, 0x06, 0x60, 0x67, 0x06, 0x3F, 0xE1, 0xFE, 0x00, 0x00, 0x00}, // d
          {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xF8, 0x3F, 0xC7, 0x06,
           0x7F, 0xE7, 0xFC, 0x60, 0x07, 0x00, 0x3F, 0xC1, 0xF8, 0x00, 0x00, 0x00}, // e
          {0x07, 0x80, 0xF8, 0x1C, 0x01, 0x80, 0x18, 0x01, 0x80, 0x7F, 0x07, 0xF0,
           0x18, 0x01, 0x80, 0x18, 0x01, 0x80, 0x18, 0x01, 0x80, 0x00, 0x00, 0x00}, // f
          {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xFE, 0x3F, 0xE7, 0x06,
           0x60, 0x67, 0x0E, 0x3F, 0xE1, 0xF6, 0x00, 0x60, 0x0E, 0x3F, 0xC3, 0xF8}, // g
          {0x60, 0x06, 0x00, 0x60, 0x06, 0x00, 0x60, 0x06, 0xF0, 0x7F, 0x87, 0x1C,
           0x60, 0xC6, 0x0C, 0x60, 0xC6, 0x0C, 0x60, 0xC6, 0x0C, 0x00, 0x00, 0x00}, // h
          {0x00, 0x00, 0x00, 0x06, 0x00, 0x60, 0x00, 0x00, 0xE0, 0x0E, 0x00, 0x60,
           0x06, 0x00, 0x60, 0x06, 0x00, 0x60, 0x1F, 0x81, 0xF8, 0x00, 0x00, 0x00}, // i
          {0x00, 0x00, 0x00, 0x01, 0x80, 0x18, 0x00, 0x00, 0x38, 0x03, 0x80, 0x18,
           0x01, 0x80, 0x18, 0x01, 0x80, 0x18, 0x01, 0x81, 0x98, 0x1F, 0x80, 0xF0}, // j
          {0x30, 0x03, 0x00, 0x30, 0x03, 0x00, 0x30, 0x03, 0x18, 0x33, 0x83, 0x70,
           0x3E, 0x03, 0xE0, 0x37, 0x03, 0x38, 0x31, 0xC3, 0x0C, 0x00, 0x00, 0x00}, // k
          {0x0E, 0x00, 0xE0, 0x06, 0x00, 0x60, 0x06, 0x00, 0x60, 0x06, 0x00, 0x60,
           0x06, 0x00, 0x60, 0x06, 0x00, 0x60, 0x1F, 0x81, 0xF8, 0x00, 0x00, 0x00}, // l
          {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x98, 0x7F, 0xC7, 0xFE,
           0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x00, 0x00, 0x00}, // m
          {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xF8, 0x3F, 0xC3, 0x0E,
           0x30, 0x63, 0x06, 0x30, 0x63, 0x06, 0x30, 0x63, 0x06, 0x00, 0x00, 0x00}, // n
          {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xF8, 0x3F, 0xC7, 0x0E,
           0x60, 0x66, 0x06, 0x60, 0x67, 0x0E, 0x3F, 0xC1, 0xF8, 0x00, 0x00, 0x00}, // o
          {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xF8, 0x7F, 0xC6, 0x0E,
           0x60, 0x66, 0x06, 0x70, 0xE7, 0xFC, 0x6F, 0x86, 0x00, 0x60, 0x06, 0x00}, // p
          {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xFE, 0x3F, 0xE7, 0x06,
           0x60, 0x66, 0x06, 0x70, 0xE3, 0xFE, 0x1F, 0x60, 0x06, 0x00, 0x60, 0x06}, // q
          {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x7C, 0x3F, 0xE3, 0x86,
           0x30, 0x03, 0x00, 0x30, 0x03, 0x00, 0x30, 0x03, 0x00, 0x00, 0x00, 0x00}, // r
          {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xF0, 0x7F, 0x86, 0x00,
           0x7F, 0x03, 0xF8, 0x01, 0x80, 0x18, 0x7F, 0x83, 0xF0, 0x00, 0x00, 0x00}, // s
          {0x00, 0x01, 0x80, 0x18, 0x01, 0x80, 0x18, 0x07, 0xF0, 0x7F, 0x01, 0x80,
           0x18, 0x01, 0x80, 0x18, 0x01, 0x80, 0x1F, 0x80, 0xF8, 0x00, 0x00, 0x00}, // t
          {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x06, 0x60, 0x66, 0x06,
           0x60, 0x66, 0x06, 0x60, 0x67, 0x0E, 0x3F, 0xE1, 0xF6, 0x00, 0x00, 0x00}, // u
          {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x06, 0x60, 0x63, 0x0C,
           0x30, 0xC1, 0x98, 0x19, 0x80, 0xF0, 0x0F, 0x00, 0x60, 0x00, 0x00, 0x00}, // v
          {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x66, 0x66, 0x66, 0x66,
           0x66, 0x66, 0x66, 0x6F, 0x63, 0xFC, 0x39, 0xC1, 0x08, 0x00, 0x00, 0x00}, // w
          {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x0C, 0x71, 0xC3, 0xB8,
           0x1F, 0x00, 0xE0, 0x1F, 0x03, 0xB8, 0x71, 0xC6, 0x0C, 0x00, 0x00, 0x00}, // x
          {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x0C, 0x30, 0xC1, 0x98,
           0x19, 0x80, 0xF0, 0x0F, 0x00, 0x60, 0x06, 0x00, 0xC0, 0x0C, 0x01, 0x80}, // y
          {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xFC, 0x7F, 0x80, 0x30,
           0x06, 0x00, 0xC0, 0x18, 0x03, 0x00, 0x7F, 0xC7, 0xFC, 0x00, 0x00, 0x00}, // z
          {0x03, 0xC0, 0x7C, 0x0E, 0x00, 0xC0, 0x0C, 0x00, 0xC0, 0x1C, 0x03, 0x80,
           0x1C, 0x00, 0xC0, 0x0C, 0x00, 0xC0, 0x0E, 0x00, 0x7C, 0x03, 0xC0, 0x00}, // {
          {0x06, 0x00, 0x60, 0x06, 0x00, 0x60, 0x06, 0x00, 0x60, 0x06, 0x00, 0x60,
           0x06, 0x00, 0x60, 0x06, 0x00, 0x60, 0x06, 0x00, 0x60, 0x00, 0x00, 0x00}, // |
          {0x3C, 0x03, 0xE0, 0x07, 0x00, 0x30, 0x03, 0x00, 0x30, 0x03, 0x80, 0x1C,
           0x03, 0x80, 0x30, 0x03, 0x00, 0x30, 0x07, 0x03, 0xE0, 0x3C, 0x00, 0x00}, // }
          {0x00, 0x00, 0x00, 0x1C, 0x63, 0x6C, 0x63, 0x80, 0x00, 0x00, 0x00, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // ~
          {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0xF0, 0x19, 0x83, 0x0C,
           0x60, 0x66, 0x06, 0x7F, 0xE7, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00} // 
  };

  const uint8_t* get_letter(const u_char letter) const override
  {
    if (letter >= 32 and letter < 128)
      // array starts at ascii 32
      return BigFont::font[letter - 32];
    return BigFont::font[1]; // return the char corresponding to '!'
  }

  inline uint8_t get_height() const override { return 16; };
  inline uint8_t get_width() const override { return 12; };
};
static const BigFont bigFont;

IFont const* font_from_scale(float& scale)
{
  scale = min(max(scale, 0.2), 1.0);
  if (scale <= 0.5)
  {
    scale = 1 - (0.5 - scale);
    return &smallFont;
  }
  else
  {
    return &bigFont;
  }
}

/**
 * \brief Display a single character
 * \param[in] letter
 * \param[in] startXIndex width at which the text starts, in lamp coordinates
 * \param[in] startYIndex height at which the text starts, in lamp coordinates
 * \param[in] scale Between 0 and 1, size of the characters
 * \param[in] color
 * \param[in, out] isDisplayedOut Indicates if this letter is completly out of bounds
 * \return the char font width
 */
uint8_t display_letter(const u_char letter,
                       const int16_t startXIndex,
                       const int16_t startYIndex,
                       float scale,
                       const Color& color,
                       bool& isDisplayedOut,
                       LedStrip& strip)
{
  uint8_t xIndex = 0;
  uint8_t yIndex = 0;

  IFont const* font = font_from_scale(scale);

  bool hasCutoffX = false; // letter was displayed out of screen, or cutout by
                           // the screen (above zero)
  isDisplayedOut = true;   // letter was displayed completly out of screen, under zero

  const auto& letterArray = font->get_letter(letter);
  const uint8_t arrayLen = font->get_arrayLenght();
  for (uint8_t i = 0; i < arrayLen; i++)
  {
    uint8_t letterPart = letterArray[i];
    // unpack the mask
    for (uint8_t j = 0; j < 8; ++j)
    {
      const int16_t targetX = startXIndex + xIndex * scale;
      const int16_t targetY = startYIndex + yIndex * scale;
      const bool isCutoff = targetX > stripXCoordinates or targetY > stripXCoordinates;
      if (not isCutoff)
      {
        const bool shouldSet = (letterPart & 0x80) != 0;
        if (shouldSet)
        {
          if (targetX >= 0 and targetY >= 0)
          {
            isDisplayedOut = false;
            const auto pixelIndex = to_strip(targetX, targetY);
            strip.setPixelColor(pixelIndex, color.get_color(pixelIndex, LED_COUNT));
          }
        }
      }
      else
      {
        isDisplayedOut = false;
      }
      // store the fact that a letter has been cutoff in the x axis
      hasCutoffX = hasCutoffX or (targetX > stripXCoordinates);

      // remove the first set bit
      letterPart = letterPart << 1;

      xIndex++;
      if (xIndex > font->get_width() - 1)
      {
        xIndex = 0;
        yIndex++;
      }
    }
  }

  if (hasCutoffX)
    return 0;
  return font->get_width();
}

bool display_text(const Color& color,
                  const std::string& text,
                  const int16_t startXIndex,
                  const int16_t startYIndex,
                  float scale,
                  const bool paddEnd,
                  LedStrip& strip)
{
  bool lastLetterDisappeared = true;
  int16_t displayColumn = startXIndex;
  for (const char c: text)
  {
    const auto res = display_letter(c, displayColumn, startYIndex, scale, color, lastLetterDisappeared, strip);
    if (res == 0) // still some letters to display
      return false;
    displayColumn += res;
  }

  // animation with padding does not stop until the last letter is gone
  if (paddEnd)
    return lastLetterDisappeared;

  return true;
}

bool display_scrolling_text(const Color& color,
                            const std::string& text,
                            const int16_t startYIndex,
                            float scale,
                            const uint32_t durationPerChar,
                            const bool reset,
                            const bool paddEnd,
                            const uint8_t fadeOut,
                            LedStrip& strip)
{
  static float xIndex = stripXCoordinates;
  if (reset)
  {
    xIndex = stripXCoordinates;
    return false;
  }

  strip.fadeToBlackBy(fadeOut);

  // slow animation: blend
  static uint32_t lastSubstep = 0;
  const uint32_t maxSubstep = MAIN_LOOP_UPDATE_PERIOD_MS / durationPerChar;

  if (display_text(color, text, xIndex, startYIndex, scale, paddEnd, strip))
  {
    return true;
  }

  const float fontSize = static_cast<float>(font_from_scale(scale)->get_width());

  const float letterIterationsToCompleteTurn =
          static_cast<float>(durationPerChar) / static_cast<float>(MAIN_LOOP_UPDATE_PERIOD_MS);
  // here, the size of the letter is ignored
  const float increment = (stripXCoordinates + fontSize) / letterIterationsToCompleteTurn;
  xIndex -= increment;

  return false;
}

} // namespace text

#endif
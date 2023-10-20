#include "noise.h"

#include "math8.h"
#include "../utils/utils.h"

namespace noise {

static uint8_t const p[] = {
    151, 160, 137,  91,  90,  15, 131,  13, 201,  95,  96,  53, 194, 233,   7, 225,
    140,  36, 103,  30,  69, 142,   8,  99,  37, 240,  21,  10,  23, 190,   6, 148,
    247, 120, 234,  75,   0,  26, 197,  62,  94, 252, 219, 203, 117,  35,  11,  32,
     57, 177,  33,  88, 237, 149,  56,  87, 174,  20, 125, 136, 171, 168,  68, 175,
     74, 165,  71, 134, 139,  48,  27, 166,  77, 146, 158, 231,  83, 111, 229, 122,
     60, 211, 133, 230, 220, 105,  92,  41,  55,  46, 245,  40, 244, 102, 143,  54,
     65,  25,  63, 161,   1, 216,  80,  73, 209,  76, 132, 187, 208,  89,  18, 169,
    200, 196, 135, 130, 116, 188, 159,  86, 164, 100, 109, 198, 173, 186,   3,  64,
     52, 217, 226, 250, 124, 123,   5, 202,  38, 147, 118, 126, 255,  82,  85, 212,
    207, 206,  59, 227,  47,  16,  58,  17, 182, 189,  28,  42, 223, 183, 170, 213,
    119, 248, 152,   2,  44, 154, 163,  70, 221, 153, 101, 155, 167,  43, 172,   9,
    129,  22,  39, 253,  19,  98, 108, 110,  79, 113, 224, 232, 178, 185, 112, 104,
    218, 246,  97, 228, 251,  34, 242, 193, 238, 210, 144,  12, 191, 179, 162, 241,
     81,  51, 145, 235, 249,  14, 239, 107,  49, 192, 214,  31, 181, 199, 106, 157,
    184,  84, 204, 176, 115, 121,  50,  45, 127,   4, 150, 254, 138, 236, 205,  93,
    222, 114,  67,  29,  24,  72, 243, 141, 128, 195,  78,  66, 215,  61, 156, 180,
    151};
uint8_t P(uint8_t x) {
  return p[x];
}



int8_t lerp7by8(int8_t a, int8_t b, fract8 frac)
{
    // int8_t delta = b - a;
    // int16_t prod = (uint16_t)delta * (uint16_t)frac;
    // int8_t scaled = prod >> 8;
    // int8_t result = a + scaled;
    // return result;
    int8_t result;
    if( b > a) {
        uint8_t delta = b - a;
        uint8_t scaled = scale8( delta, frac);
        result = a + scaled;
    } else {
        uint8_t delta = a - b;
        uint8_t scaled = scale8( delta, frac);
        result = a - scaled;
    }
    return result;
}

int8_t grad8(uint8_t hash, int8_t x)
{
    // since the tests below can be done bit-wise on the bottom
    // four bits, there's no need to mask off the higher bits
    //	hash = hash & 15;

    int8_t u,v;
    if(hash & 8) {
        u=x; v=x;
    } else {
    if(hash & 4) {
        u=1; v=x;
    } else {
        u=x; v=1;
    }
    }

    if(hash&1) { u = -u; }
    if(hash&2) { v = -v; }

    return avg7(u,v);
}

int8_t inline grad8(uint8_t hash, int8_t x, int8_t y)
{
    // since the tests below can be done bit-wise on the bottom
    // three bits, there's no need to mask off the higher bits
    //  hash = hash & 7;

    int8_t u,v;
    if( hash & 4) {
        u = y; v = x;
    } else {
        u = x; v = y;
    }

    if(hash&1) { u = -u; }
    if(hash&2) { v = -v; }

    return avg7(u,v);
}

int8_t inoise8_raw(uint16_t x)
{
  // Find the unit cube containing the point
  uint8_t X = x>>8;

  // Hash cube corner coordinates
  uint8_t A = P(X);
  uint8_t AA = P(A);
  uint8_t B = P(X+1);
  uint8_t BA = P(B);

  // Get the relative position of the point in the cube
  uint8_t u = x;

  // Get a signed version of the above for the grad function
  int8_t xx = ((uint8_t)(x)>>1) & 0x7F;
  uint8_t N = 0x80;

  u = FADE( u);
  
  int8_t ans = lerp7by8(grad8(P(AA), xx), grad8(P(BA), xx - N), u);

  return ans;
}

int8_t inoise8_raw(uint16_t x, uint16_t y)
{
    // Find the unit cube containing the point
    uint8_t X = x>>8;
    uint8_t Y = y>>8;

    // Hash cube corner coordinates
    uint8_t A = P(X)+Y;
    uint8_t AA = P(A);
    uint8_t AB = P(A+1);
    uint8_t B = P(X+1)+Y;
    uint8_t BA = P(B);
    uint8_t BB = P(B+1);

    // Get the relative position of the point in the cube
    uint8_t u = x;
    uint8_t v = y;

    // Get a signed version of the above for the grad function
    int8_t xx = ((uint8_t)(x)>>1) & 0x7F;
    int8_t yy = ((uint8_t)(y)>>1) & 0x7F;
    uint8_t N = 0x80;

    u = FADE(u); v = FADE(v);

    int8_t X1 = lerp7by8(grad8(P(AA), xx, yy), grad8(P(BA), xx - N, yy), u);
    int8_t X2 = lerp7by8(grad8(P(AB), xx, yy-N), grad8(P(BB), xx - N, yy - N), u);

    int8_t ans = lerp7by8(X1,X2,v);

    return ans;
    // return scale8((70+(ans)),234)<<1;
}

uint8_t inoise8(uint16_t x) {
    int8_t n = inoise8_raw(x);    //-64..+64
    n += 64;                      // 0..128
    uint8_t ans = qadd8(n,n);     // 0..255
    return ans;
}

uint8_t inoise8(uint16_t x, uint16_t y) {
  //return scale8(69+inoise8_raw(x,y),237)<<1;
    int8_t n = inoise8_raw( x, y);  // -64..+64
    n+= 64;                         //   0..128
    uint8_t ans = qadd8( n, n);     //   0..255
    return ans;
}

uint8_t inoise8_octaves(uint16_t x, uint8_t octaves, int scale, uint16_t time) {
    uint8_t noise = 0;
    uint32_t _xx = x;
    uint32_t scx = scale;
    for(int o = 0; o < octaves; ++o) {
        noise = qadd8(noise,inoise8(_xx + scx, time)>>o);

        _xx <<= 1;
        scx <<= 1;
    }
    return noise;
}


}

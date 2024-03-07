 
#ifndef __INC_LIB8TION_MATH_H
#define __INC_LIB8TION_MATH_H

#include "scale8.h"

#include <Arduino.h> //PI constant

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif
#ifndef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))
#endif

/// @file math8.h
/// Fast, efficient 8-bit math functions specifically
/// designed for high-performance LED programming. 

/// @ingroup lib8tion
/// @{

/// @defgroup Math Basic Math Operations
/// Fast, efficient 8-bit math functions specifically
/// designed for high-performance LED programming.
///
/// Because of the AVR (Arduino) and ARM assembly language
/// implementations provided, using these functions often
/// results in smaller and faster code than the equivalent
/// program using plain "C" arithmetic and logic.
/// @{

/// Add one byte to another, saturating at 0xFF
/// @param i first byte to add
/// @param j second byte to add
/// @returns the sum of i + j, capped at 0xFF
LIB8STATIC_ALWAYS_INLINE uint8_t qadd8( uint8_t i, uint8_t j)
{
#if QADD8_C == 1
    unsigned int t = i + j;
    if( t > 255) t = 255;
    return t;
#elif QADD8_AVRASM == 1
    asm volatile(
        /* First, add j to i, conditioning the C flag */
        "add %0, %1    \n\t"

        /* Now test the C flag.
        If C is clear, we branch around a load of 0xFF into i.
        If C is set, we go ahead and load 0xFF into i.
        */
        "brcc L_%=     \n\t"
        "ldi %0, 0xFF  \n\t"
        "L_%=: "
        : "+a" (i)
        : "a"  (j)
    );
    return i;
#elif QADD8_ARM_DSP_ASM == 1
    asm volatile( "uqadd8 %0, %0, %1" : "+r" (i) : "r" (j));
    return i;
#else
#error "No implementation for qadd8 available."
#endif
}

/// Add one byte to another, saturating at 0x7F and -0x80
/// @param i first byte to add
/// @param j second byte to add
/// @returns the sum of i + j, capped at 0x7F and -0x80
LIB8STATIC_ALWAYS_INLINE int8_t qadd7( int8_t i, int8_t j)
{
#if QADD7_C == 1
    int16_t t = i + j;
    if( t > 127) t = 127;
    else if( t < -128) t = -128;
    return t;
#elif QADD7_AVRASM == 1
    asm volatile(
        /* First, add j to i, conditioning the V and C flags */
        "add %0, %1    \n\t"

        /* Now test the V flag.
        If V is clear, we branch to end.
        If V is set, we go ahead and load 0x7F into i.
        */
        "brvc L_%=     \n\t"
        "ldi %0, 0x7F  \n\t"

        /* When both numbers are negative, C is set.
        Adding it to make result negative. */
        "adc %0, __zero_reg__\n\t"
        "L_%=: "
        : "+a" (i)
        : "a"  (j)
    );
    return i;
#elif QADD7_ARM_DSP_ASM == 1
    asm volatile( "qadd8 %0, %0, %1" : "+r" (i) : "r" (j));
    return i;
#else
#error "No implementation for qadd7 available."
#endif
}

/// Subtract one byte from another, saturating at 0x00
/// @param i byte to subtract from
/// @param j byte to subtract
/// @returns i - j with a floor of 0
LIB8STATIC_ALWAYS_INLINE uint8_t qsub8( uint8_t i, uint8_t j)
{
#if QSUB8_C == 1
    int t = i - j;
    if( t < 0) t = 0;
    return t;
#elif QSUB8_AVRASM == 1

    asm volatile(
        /* First, subtract j from i, conditioning the C flag */
        "sub %0, %1    \n\t"

        /* Now test the C flag.
        If C is clear, we branch around a load of 0x00 into i.
        If C is set, we go ahead and load 0x00 into i.
        */
        "brcc L_%=     \n\t"
        "ldi %0, 0x00  \n\t"
        "L_%=: "
        : "+a" (i)
        : "a"  (j)
    );
    return i;
#else
#error "No implementation for qsub8 available."
#endif
}

/// Add one byte to another, with 8-bit result
/// @note This does not saturate and may overflow!
/// @param i first byte to add
/// @param j second byte to add
/// @returns the sum of i + j, 8-bit
LIB8STATIC_ALWAYS_INLINE uint8_t add8( uint8_t i, uint8_t j)
{
#if ADD8_C == 1
    int t = i + j;
    return t;
#elif ADD8_AVRASM == 1
    // Add j to i, period.
    asm volatile( "add %0, %1" : "+a" (i) : "a" (j));
    return i;
#else
#error "No implementation for add8 available."
#endif
}

/// Add one byte to two bytes, with 16-bit result
/// @note This does not saturate and may overflow!
/// @param i first value to add, 8-bit
/// @param j second value to add, 16-bit
/// @returns the sum of i + j, 16-bit
LIB8STATIC_ALWAYS_INLINE uint16_t add8to16( uint8_t i, uint16_t j)
{
#if ADD8_C == 1
    uint16_t t = i + j;
    return t;
#elif ADD8_AVRASM == 1
    // Add i(one byte) to j(two bytes)
    asm volatile(
        "add %A[j], %[i]              \n\t"
        "adc %B[j], __zero_reg__      \n\t"
        : [j] "+a" (j)
        : [i] "a"  (i)
    );
    return i;
#else
#error "No implementation for add8to16 available."
#endif
}


/// Subtract one byte from another, 8-bit result
/// @note This does not saturate and may overflow!
/// @param i byte to subtract from
/// @param j byte to subtract
/// @returns i - j
LIB8STATIC_ALWAYS_INLINE uint8_t sub8( uint8_t i, uint8_t j)
{
#if SUB8_C == 1
    int t = i - j;
    return t;
#elif SUB8_AVRASM == 1
    // Subtract j from i, period.
    asm volatile( "sub %0, %1" : "+a" (i) : "a" (j));
    return i;
#else
#error "No implementation for sub8 available."
#endif
}

/// Calculate an integer average of two unsigned
/// 8-bit integer values (uint8_t), rounded down. 
/// Fractional results are rounded down, e.g. avg8(20,41) = 30
/// @param i first value to average
/// @param j second value to average
/// @returns mean average of i and j, rounded down
LIB8STATIC_ALWAYS_INLINE uint8_t avg8( uint8_t i, uint8_t j)
{
#if AVG8_C == 1
    return (i + j) >> 1;
#elif AVG8_AVRASM == 1
    asm volatile(
        /* First, add j to i, 9th bit overflows into C flag */
        "add %0, %1    \n\t"
        /* Divide by two, moving C flag into high 8th bit */
        "ror %0        \n\t"
        : "+a" (i)
        : "a"  (j)
    );
    return i;
#else
#error "No implementation for avg8 available."
#endif
}

/// Calculate an integer average of two unsigned
/// 16-bit integer values (uint16_t), rounded down. 
/// Fractional results are rounded down, e.g. avg16(20,41) = 30
/// @param i first value to average
/// @param j second value to average
/// @returns mean average of i and j, rounded down
LIB8STATIC_ALWAYS_INLINE uint16_t avg16( uint16_t i, uint16_t j)
{
#if AVG16_C == 1
    return (uint32_t)((uint32_t)(i) + (uint32_t)(j)) >> 1;
#elif AVG16_AVRASM == 1
    asm volatile(
        /* First, add jLo (heh) to iLo, 9th bit overflows into C flag */
        "add %A[i], %A[j]    \n\t"
        /* Now, add C + jHi to iHi, 17th bit overflows into C flag */
        "adc %B[i], %B[j]    \n\t"
        /* Divide iHi by two, moving C flag into high 16th bit, old 9th bit now in C */
        "ror %B[i]        \n\t"
        /* Divide iLo by two, moving C flag into high 8th bit */
        "ror %A[i]        \n\t"
        : [i] "+a" (i)
        : [j] "a"  (j)
    );
    return i;
#else
#error "No implementation for avg16 available."
#endif
}

/// Calculate an integer average of two unsigned
/// 8-bit integer values (uint8_t), rounded up. 
/// Fractional results are rounded up, e.g. avg8r(20,41) = 31
/// @param i first value to average
/// @param j second value to average
/// @returns mean average of i and j, rounded up
LIB8STATIC_ALWAYS_INLINE uint8_t avg8r( uint8_t i, uint8_t j)
{
#if AVG8R_C == 1
    return (i + j + 1) >> 1;
#elif AVG8R_AVRASM == 1
    asm volatile(
        /* First, add j to i, 9th bit overflows into C flag */
        "add %0, %1          \n\t"
        /* Divide by two, moving C flag into high 8th bit, old 1st bit now in C */
        "ror %0              \n\t"
        /* Add C flag */
        "adc %0, __zero_reg__\n\t"
        : "+a" (i)
        : "a"  (j)
    );
    return i;
#else
#error "No implementation for avg8r available."
#endif
}

/// Calculate an integer average of two unsigned
/// 16-bit integer values (uint16_t), rounded up. 
/// Fractional results are rounded up, e.g. avg16r(20,41) = 31
/// @param i first value to average
/// @param j second value to average
/// @returns mean average of i and j, rounded up
LIB8STATIC_ALWAYS_INLINE uint16_t avg16r( uint16_t i, uint16_t j)
{
#if AVG16R_C == 1
    return (uint32_t)((uint32_t)(i) + (uint32_t)(j) + 1) >> 1;
#elif AVG16R_AVRASM == 1
    asm volatile(
        /* First, add jLo (heh) to iLo, 9th bit overflows into C flag */
        "add %A[i], %A[j]    \n\t"
        /* Now, add C + jHi to iHi, 17th bit overflows into C flag */
        "adc %B[i], %B[j]    \n\t"
        /* Divide iHi by two, moving C flag into high 16th bit, old 9th bit now in C */
        "ror %B[i]        \n\t"
        /* Divide iLo by two, moving C flag into high 8th bit, old 1st bit now in C */
        "ror %A[i]        \n\t"
        /* Add C flag */
        "adc %A[i], __zero_reg__\n\t"
        "adc %B[i], __zero_reg__\n\t"
        : [i] "+a" (i)
        : [j] "a"  (j)
    );
    return i;
#else
#error "No implementation for avg16r available."
#endif
}


/// Calculate an integer average of two signed 7-bit
/// integers (int8_t). 
/// If the first argument is even, result is rounded down. 
/// If the first argument is odd, result is rounded up. 
/// @param i first value to average
/// @param j second value to average
/// @returns mean average of i and j, rounded
LIB8STATIC_ALWAYS_INLINE int8_t avg7( int8_t i, int8_t j)
{
#if AVG7_C == 1
    return (i>>1) + (j>>1) + (i & 0x1);
#elif AVG7_AVRASM == 1
    asm volatile(
        "asr %1        \n\t"
        "asr %0        \n\t"
        "adc %0, %1    \n\t"
        : "+a" (i)
        : "a"  (j)
    );
    return i;
#else
#error "No implementation for avg7 available."
#endif
}

/// Calculate an integer average of two signed 15-bit
/// integers (int16_t). 
/// If the first argument is even, result is rounded down.
/// If the first argument is odd, result is rounded up.
/// @param i first value to average
/// @param j second value to average
/// @returns mean average of i and j, rounded
LIB8STATIC_ALWAYS_INLINE int16_t avg15( int16_t i, int16_t j)
{
#if AVG15_C == 1
    return (i>>1) + (j>>1) + (i & 0x1);
#elif AVG15_AVRASM == 1
    asm volatile(
        /* first divide j by 2, throwing away lowest bit */
        "asr %B[j]          \n\t"
        "ror %A[j]          \n\t"
        /* now divide i by 2, with lowest bit going into C */
        "asr %B[i]          \n\t"
        "ror %A[i]          \n\t"
        /* add j + C to i */
        "adc %A[i], %A[j]   \n\t"
        "adc %B[i], %B[j]   \n\t"
        : [i] "+a" (i)
        : [j] "a"  (j)
    );
    return i;
#else
#error "No implementation for avg15 available."
#endif
}


/// Calculate the remainder of one unsigned 8-bit
/// value divided by anoter, aka A % M. 
/// Implemented by repeated subtraction, which is
/// very compact, and very fast if A is "probably"
/// less than M.  If A is a large multiple of M,
/// the loop has to execute multiple times.  However,
/// even in that case, the loop is only two
/// instructions long on AVR, i.e., quick.
/// @param a dividend byte
/// @param m divisor byte
/// @returns remainder of a / m (i.e. a % m)
LIB8STATIC_ALWAYS_INLINE uint8_t mod8( uint8_t a, uint8_t m)
{
#if defined(__AVR__)
    asm volatile (
        "L_%=:  sub %[a],%[m]    \n\t"
        "       brcc L_%=        \n\t"
        "       add %[a],%[m]    \n\t"
        : [a] "+r" (a)
        : [m] "r"  (m)
    );
#else
    while( a >= m) a -= m;
#endif
    return a;
}

/// Add two numbers, and calculate the modulo
/// of the sum and a third number, M. 
/// In other words, it returns (A+B) % M.
/// It is designed as a compact mechanism for
/// incrementing a "mode" switch and wrapping
/// around back to "mode 0" when the switch
/// goes past the end of the available range.
/// e.g. if you have seven modes, this switches
/// to the next one and wraps around if needed:
///   @code{.cpp}
///   mode = addmod8( mode, 1, 7);
///   @endcode
/// @param a dividend byte
/// @param b value to add to the dividend
/// @param m divisor byte
/// @returns remainder of (a + b) / m
/// @see mod8() for notes on performance.
LIB8STATIC uint8_t addmod8( uint8_t a, uint8_t b, uint8_t m)
{
#if defined(__AVR__)
    asm volatile (
        "       add %[a],%[b]    \n\t"
        "L_%=:  sub %[a],%[m]    \n\t"
        "       brcc L_%=        \n\t"
        "       add %[a],%[m]    \n\t"
        : [a] "+r" (a)
        : [b] "r"  (b), [m] "r" (m)
    );
#else
    a += b;
    while( a >= m) a -= m;
#endif
    return a;
}

/// Subtract two numbers, and calculate the modulo
/// of the difference and a third number, M. 
/// In other words, it returns (A-B) % M.
/// It is designed as a compact mechanism for
/// decrementing a "mode" switch and wrapping
/// around back to "mode 0" when the switch
/// goes past the start of the available range.
/// e.g. if you have seven modes, this switches
/// to the previous one and wraps around if needed:
///   @code{.cpp}
///   mode = submod8( mode, 1, 7);
///   @endcode
/// @param a dividend byte
/// @param b value to subtract from the dividend
/// @param m divisor byte
/// @returns remainder of (a - b) / m
/// @see mod8() for notes on performance.
LIB8STATIC uint8_t submod8( uint8_t a, uint8_t b, uint8_t m)
{
#if defined(__AVR__)
    asm volatile (
        "       sub %[a],%[b]    \n\t"
        "L_%=:  sub %[a],%[m]    \n\t"
        "       brcc L_%=        \n\t"
        "       add %[a],%[m]    \n\t"
        : [a] "+r" (a)
        : [b] "r"  (b), [m] "r" (m)
    );
#else
    a -= b;
    while( a >= m) a -= m;
#endif
    return a;
}

/// 8x8 bit multiplication, with 8-bit result. 
/// @param i first byte to multiply
/// @param j second byte to multiply
/// @returns the product of i * j
/// @note This does not saturate and may overflow!
LIB8STATIC_ALWAYS_INLINE uint8_t mul8( uint8_t i, uint8_t j)
{
#if MUL8_C == 1
    return ((int)i * (int)(j) ) & 0xFF;
#elif MUL8_AVRASM == 1
    asm volatile(
        /* Multiply 8-bit i * 8-bit j, giving 16-bit r1,r0 */
        "mul %0, %1          \n\t"
        /* Extract the LOW 8-bits (r0) */
        "mov %0, r0          \n\t"
        /* Restore r1 to "0"; it's expected to always be that */
        "clr __zero_reg__    \n\t"
        : "+a" (i)
        : "a"  (j)
        : "r0", "r1"
    );
    return i;
#else
#error "No implementation for mul8 available."
#endif
}


/// 8x8 bit multiplication with 8-bit result, saturating at 0xFF. 
/// @param i first byte to multiply
/// @param j second byte to multiply
/// @returns the product of i * j, capping at 0xFF
LIB8STATIC_ALWAYS_INLINE uint8_t qmul8( uint8_t i, uint8_t j)
{
#if QMUL8_C == 1
    unsigned p = (unsigned)i * (unsigned)j;
    if( p > 255) p = 255;
    return p;
#elif QMUL8_AVRASM == 1
    asm volatile(
        /* Multiply 8-bit i * 8-bit j, giving 16-bit r1,r0 */
        "  mul %0, %1          \n\t"
        /* Extract the LOW 8-bits (r0) */
        "  mov %0, r0          \n\t"
        /* If high byte of result is zero, all is well. */
        "  tst r1              \n\t"
        "  breq Lnospill_%=    \n\t"
        /* If high byte of result > 0, saturate to 0xFF */
        "  ldi %0, 0xFF         \n\t"
        "Lnospill_%=:          \n\t"
        /* Restore r1 to "0"; it's expected to always be that */
        "  clr __zero_reg__    \n\t"
        : "+a" (i)
        : "a"  (j)
        : "r0", "r1"
    );
    return i;
#else
#error "No implementation for qmul8 available."
#endif
}


/// Take the absolute value of a signed 8-bit uint8_t. 
LIB8STATIC_ALWAYS_INLINE int8_t abs8( int8_t i)
{
#if ABS8_C == 1
    if( i < 0) i = -i;
    return i;
#elif ABS8_AVRASM == 1
    asm volatile(
        /* First, check the high bit, and prepare to skip if it's clear */
        "sbrc %0, 7 \n"

        /* Negate the value */
        "neg %0     \n"

        : "+r" (i) : "r" (i)
    );
    return i;
#else
#error "No implementation for abs8 available."
#endif
}

/// Square root for 16-bit integers. 
/// About three times faster and five times smaller
/// than Arduino's general `sqrt` on AVR.
LIB8STATIC uint8_t sqrt16(uint16_t x)
{
    if( x <= 1) {
        return x;
    }

    uint8_t low = 1; // lower bound
    uint8_t hi, mid;

    if( x > 7904) {
        hi = 255;
    } else {
        hi = (x >> 5) + 8; // initial estimate for upper bound
    }

    do {
        mid = (low + hi) >> 1;
        if ((uint16_t)(mid * mid) > x) {
            hi = mid - 1;
        } else {
            if( mid == 255) {
                return 255;
            }
            low = mid + 1;
        }
    } while (hi >= low);

    return low - 1;
}

/// Blend a variable proportion (0-255) of one byte to another. 
/// @param a the starting byte value
/// @param b the byte value to blend toward
/// @param amountOfB the proportion (0-255) of b to blend
/// @returns a byte value between a and b, inclusive
#if (FASTLED_BLEND_FIXED == 1)
LIB8STATIC uint8_t blend8( uint8_t a, uint8_t b, uint8_t amountOfB)
{

    // The BLEND_FIXED formula is
    //
    //   result = (  A*(amountOfA) + B*(amountOfB)              )/ 256
    //
    // …where amountOfA = 255-amountOfB.
    //
    // This formula will never return 255, which is why the BLEND_FIXED + SCALE8_FIXED version is
    //
    //   result = (  A*(amountOfA) + A + B*(amountOfB) + B      ) / 256
    //
    // We can rearrange this formula for some great optimisations.
    //
    //   result = (  A*(amountOfA) + A + B*(amountOfB) + B      ) / 256
    //          = (  A*(255-amountOfB) + A + B*(amountOfB) + B  ) / 256
    //          = (  A*(256-amountOfB) + B*(amountOfB) + B      ) / 256
    //          = (  A*256 + B + B*(amountOfB) - A*(amountOfB)  ) / 256  // this is the version used in SCALE8_FIXED AVR below
    //          = (  A*256 + B + (B-A)*(amountOfB)              ) / 256  // this is the version used in SCALE8_FIXED C below

    uint16_t partial;
    uint8_t result;

#if BLEND8_C == 1

#   if (FASTLED_SCALE8_FIXED == 1)
    partial = (a << 8) | b; // A*256 + B

    // on many platforms this compiles to a single multiply of (B-A) * amountOfB
    partial += (b * amountOfB);
    partial -= (a * amountOfB);

#   else
    uint8_t amountOfA = 255 - amountOfB;

    // on the other hand, this compiles to two multiplies, and gives the "wrong" answer :]
    partial = (a * amountOfA);
    partial += (b * amountOfB);
#   endif
    
    result = partial >> 8;
    
    return result;

#elif BLEND8_AVRASM == 1

#   if (FASTLED_SCALE8_FIXED == 1)

    // 1 or 2 cycles depending on how the compiler optimises
    partial = (a << 8) | b;

    // 7 cycles
    asm volatile (
        "  mul %[a], %[amountOfB]        \n\t"
        "  sub %A[partial], r0           \n\t"
        "  sbc %B[partial], r1           \n\t"
        "  mul %[b], %[amountOfB]        \n\t"
        "  add %A[partial], r0           \n\t"
        "  adc %B[partial], r1           \n\t"
        "  clr __zero_reg__              \n\t"
        : [partial] "+r" (partial)
        : [amountOfB] "r" (amountOfB),
          [a] "r" (a),
          [b] "r" (b)
        : "r0", "r1"
    );

#   else

    // non-SCALE8-fixed version

    // 7 cycles
    asm volatile (
        /* partial = b * amountOfB */
        "  mul %[b], %[amountOfB]        \n\t"
        "  movw %A[partial], r0          \n\t"

        /* amountOfB (aka amountOfA) = 255 - amountOfB */
        "  com %[amountOfB]              \n\t"

        /* partial += a * amountOfB (aka amountOfA) */
        "  mul %[a], %[amountOfB]        \n\t"

        "  add %A[partial], r0           \n\t"
        "  adc %B[partial], r1           \n\t"
                  
        "  clr __zero_reg__              \n\t"
                        
        : [partial] "=r" (partial),
          [amountOfB] "+a" (amountOfB)
        : [a] "a" (a),
          [b] "a" (b)
        : "r0", "r1"
    );

#   endif
    
    result = partial >> 8;
    
    return result;
    
#else
#   error "No implementation for blend8 available."
#endif
}

#else
LIB8STATIC uint8_t blend8( uint8_t a, uint8_t b, uint8_t amountOfB)
{
    // This version loses precision in the integer math
    // and can actually return results outside of the range
    // from a to b.  Its use is not recommended.
    uint8_t result;
    uint8_t amountOfA = 255 - amountOfB;
    result = scale8_LEAVING_R1_DIRTY( a, amountOfA)
           + scale8_LEAVING_R1_DIRTY( b, amountOfB);
    cleanup_R1();
    return result;
}
#endif

LIB8STATIC_ALWAYS_INLINE int16_t sin16( uint16_t theta )
{
    static const uint16_t base[] =
    { 0, 6393, 12539, 18204, 23170, 27245, 30273, 32137 };
    static const uint8_t slope[] =
    { 49, 48, 44, 38, 31, 23, 14, 4 };

    uint16_t offset = (theta & 0x3FFF) >> 3; // 0..2047
    if( theta & 0x4000 ) offset = 2047 - offset;

    uint8_t section = offset / 256; // 0..7
    uint16_t b   = base[section];
    uint8_t  m   = slope[section];

    uint8_t secoffset8 = (uint8_t)(offset) / 2;

    uint16_t mx = m * secoffset8;
    int16_t  y  = mx + b;

    if( theta & 0x8000 ) y = -y;

    return y;
}

LIB8STATIC_ALWAYS_INLINE int16_t cos16( uint16_t theta)
{
    return sin16( theta + 16384);
}

/// Pre-calculated lookup table used in sin8() and cos8() functions
const uint8_t b_m16_interleave[] = { 0, 49, 49, 41, 90, 27, 117, 10 };
LIB8STATIC uint8_t sin8(uint8_t theta)
{
    uint8_t offset = theta;
    if( theta & 0x40 ) {
        offset = (uint8_t)255 - offset;
    }
    offset &= 0x3F; // 0..63

    uint8_t secoffset  = offset & 0x0F; // 0..15
    if( theta & 0x40) ++secoffset;

    uint8_t section = offset >> 4; // 0..3
    uint8_t s2 = section * 2;
    const uint8_t* p = b_m16_interleave;
    p += s2;
    uint8_t b   =  *p;
    ++p;
    uint8_t m16 =  *p;

    uint8_t mx = (m16 * secoffset) >> 4;

    int8_t y = mx + b;
    if( theta & 0x80 ) y = -y;

    y += 128;

    return y;
}

/// Fast 8-bit approximation of cos(x). This approximation never varies more than
/// 2% from the floating point value you'd get by doing
///   @code{.cpp}
///   float s = (cos(x) * 128.0) + 128;
///   @endcode
///
/// @param theta input angle from 0-255
/// @returns cos of theta, value between 0 and 255
LIB8STATIC uint8_t cos8( uint8_t theta)
{
    return sin8( theta + 64);
}

LIB8STATIC_ALWAYS_INLINE uint8_t ease8InOutQuad( uint8_t i)
{
    uint8_t j = i;
    if( j & 0x80 ) {
        j = 255 - j;
    }
    uint8_t jj  = scale8(  j, j);
    uint8_t jj2 = jj << 1;
    if( i & 0x80 ) {
        jj2 = 255 - jj2;
    }
    return jj2;
}

LIB8STATIC_ALWAYS_INLINE fract8 ease8InOutCubic( fract8 i)
{
    uint8_t ii  = scale8_LEAVING_R1_DIRTY(  i, i);
    uint8_t iii = scale8_LEAVING_R1_DIRTY( ii, i);

    uint16_t r1 = (3 * (uint16_t)(ii)) - ( 2 * (uint16_t)(iii));

    /* the code generated for the above *'s automatically
       cleans up R1, so there's no need to explicitily call
       cleanup_R1(); */

    uint8_t result = r1;

    // if we got "256", return 255:
    if( r1 & 0x100 ) {
        result = 255;
    }
    return result;
}

/// Triangle wave generator. 
/// Useful for turning a one-byte ever-increasing value into a
/// one-byte value that oscillates up and down.
///   @code
///           input         output
///           0..127        0..254 (positive slope)
///           128..255      254..0 (negative slope)
///   @endcode
///
/// On AVR this function takes just three cycles.
///
LIB8STATIC_ALWAYS_INLINE uint8_t triwave8(uint8_t in)
{
    if( in & 0x80) {
        in = 255 - in;
    }
    uint8_t out = in << 1;
    return out;
}

/// Quadratic waveform generator. Spends just a little
/// more time at the limits than "sine" does. 
///
/// S-shaped wave generator (like "sine"). Useful 
/// for turning a one-byte "counter" value into a
/// one-byte oscillating value that moves smoothly up and down,
/// with an "acceleration" and "deceleration" curve.
///
/// This is even faster than "sin8()", and has
/// a slightly different curve shape.
LIB8STATIC_ALWAYS_INLINE uint8_t quadwave8(uint8_t in)
{
    return ease8InOutQuad( triwave8( in));
}

/// Cubic waveform generator. Spends visibly more time
/// at the limits than "sine" does. 
/// @copydetails quadwave8()
LIB8STATIC_ALWAYS_INLINE uint8_t cubicwave8(uint8_t in)
{
    return ease8InOutCubic( triwave8( in));
}


#define modd(x, y) ((x) - (int)((x) / (y)) * (y))

LIB8STATIC_ALWAYS_INLINE float cos_t(float phi)
{
  float x = modd(phi, TWO_PI);
  if (x < 0) x = -1 * x;
  int8_t sign = 1;
  if (x > PI)
  {
      x -= PI;
      sign = -1;
  }
  float xx = x * x;

  float res = sign * (1 - ((xx) / (2)) + ((xx * xx) / (24)) - ((xx * xx * xx) / (720)) + ((xx * xx * xx * xx) / (40320)) - ((xx * xx * xx * xx * xx) / (3628800)) + ((xx * xx * xx * xx * xx * xx) / (479001600)));
  #ifdef WLED_DEBUG_MATH
  Serial.printf("cos: %f,%f,%f,(%f)\n",phi,res,cos(x),res-cos(x));
  #endif
  return res;
}

LIB8STATIC_ALWAYS_INLINE float sin_t(float x) {
  float res =  cos_t(HALF_PI - x);
  #ifdef WLED_DEBUG_MATH
  Serial.printf("sin: %f,%f,%f,(%f)\n",x,res,sin(x),res-sin(x));
  #endif
  return res;
}

LIB8STATIC_ALWAYS_INLINE float tan_t(float x) {
  float c = cos_t(x);
  if (c==0.0f) return 0;
  float res = sin_t(x) / c;
  #ifdef WLED_DEBUG_MATH
  Serial.printf("tan: %f,%f,%f,(%f)\n",x,res,tan(x),res-tan(x));
  #endif
  return res;
}
///////////////////////////////////////////////////////////////////////
///
/// @defgroup BeatGenerators Waveform Beat Generators
/// Waveform generators that reset at a given number
/// of "beats per minute" (BPM).
///
/// The standard "beat" functions generate "sawtooth" waves which rise from
/// 0 up to a max value and then reset, continuously repeating that cycle at
/// the specified frequency (BPM).
///
/// The "sin" versions function similarly, but create an oscillating sine wave
/// at the specified frequency.
///
/// BPM can be supplied two ways. The simpler way of specifying BPM is as
/// a simple 8-bit integer from 1-255, (e.g., "120").
/// The more sophisticated way of specifying BPM allows for fractional
/// "Q8.8" fixed point number (an ::accum88) with an 8-bit integer part and
/// an 8-bit fractional part.  The easiest way to construct this is to multiply
/// a floating point BPM value (e.g. 120.3) by 256, (e.g. resulting in 30796
/// in this case), and pass that as the 16-bit BPM argument.
///
/// Originally these functions were designed to make an entire animation project pulse.
/// with brightness. For that effect, add this line just above your existing call to
/// "FastLED.show()":
///   @code
///   uint8_t bright = beatsin8( 60 /*BPM*/, 192 /*dimmest*/, 255 /*brightest*/ ));
///   FastLED.setBrightness( bright );
///   FastLED.show();
///   @endcode
///
/// The entire animation will now pulse between brightness 192 and 255 once per second.
///
/// @warning Any "BPM88" parameter **MUST** always be provided in Q8.8 format!
/// @note The beat generators need access to a millisecond counter
/// to track elapsed time. See ::GET_MILLIS for reference. When using the Arduino
/// `millis()` function, accuracy is a bit better than one part in a thousand.
///
/// @{


/// Generates a 16-bit "sawtooth" wave at a given BPM, with BPM
/// specified in Q8.8 fixed-point format.
/// @param beats_per_minute_88 the frequency of the wave, in Q8.8 format
/// @param timebase the time offset of the wave from the millis() timer
/// @warning The BPM parameter **MUST** be provided in Q8.8 format! E.g.
/// for 120 BPM it would be 120*256 = 30720. If you just want to specify
/// "120", use beat16() or beat8().
LIB8STATIC uint16_t beat88( accum88 beats_per_minute_88, uint32_t timebase = 0)
{
    // BPM is 'beats per minute', or 'beats per 60000ms'.
    // To avoid using the (slower) division operator, we
    // want to convert 'beats per 60000ms' to 'beats per 65536ms',
    // and then use a simple, fast bit-shift to divide by 65536.
    //
    // The ratio 65536:60000 is 279.620266667:256; we'll call it 280:256.
    // The conversion is accurate to about 0.05%, more or less,
    // e.g. if you ask for "120 BPM", you'll get about "119.93".
    return (((millis()) - timebase) * beats_per_minute_88 * 280) >> 16;
}

/// Generates a 16-bit "sawtooth" wave at a given BPM
/// @param beats_per_minute the frequency of the wave, in decimal
/// @param timebase the time offset of the wave from the millis() timer
LIB8STATIC uint16_t beat16( accum88 beats_per_minute, uint32_t timebase = 0)
{
    // Convert simple 8-bit BPM's to full Q8.8 accum88's if needed
    if( beats_per_minute < 256) beats_per_minute <<= 8;
    return beat88(beats_per_minute, timebase);
}

/// Generates an 8-bit "sawtooth" wave at a given BPM
/// @param beats_per_minute the frequency of the wave, in decimal
/// @param timebase the time offset of the wave from the millis() timer
LIB8STATIC uint8_t beat8( accum88 beats_per_minute, uint32_t timebase = 0)
{
    return beat16( beats_per_minute, timebase) >> 8;
}


/// Generates a 16-bit sine wave at a given BPM that oscillates within
/// a given range.
/// @param beats_per_minute_88 the frequency of the wave, in Q8.8 format
/// @param lowest the lowest output value of the sine wave
/// @param highest the highest output value of the sine wave
/// @param timebase the time offset of the wave from the millis() timer
/// @param phase_offset phase offset of the wave from the current position
/// @warning The BPM parameter **MUST** be provided in Q8.8 format! E.g.
/// for 120 BPM it would be 120*256 = 30720. If you just want to specify
/// "120", use beatsin16() or beatsin8().
LIB8STATIC uint16_t beatsin88( accum88 beats_per_minute_88, uint16_t lowest = 0, uint16_t highest = 65535,
                              uint32_t timebase = 0, uint16_t phase_offset = 0)
{
    uint16_t beat = beat88( beats_per_minute_88, timebase);
    uint16_t beatsin = (sin16( beat + phase_offset) + 32768);
    uint16_t rangewidth = highest - lowest;
    uint16_t scaledbeat = scale16( beatsin, rangewidth);
    uint16_t result = lowest + scaledbeat;
    return result;
}

/// Generates a 16-bit sine wave at a given BPM that oscillates within
/// a given range.
/// @param beats_per_minute the frequency of the wave, in decimal
/// @param lowest the lowest output value of the sine wave
/// @param highest the highest output value of the sine wave
/// @param timebase the time offset of the wave from the millis() timer
/// @param phase_offset phase offset of the wave from the current position
LIB8STATIC uint16_t beatsin16( accum88 beats_per_minute, uint16_t lowest = 0, uint16_t highest = 65535,
                               uint32_t timebase = 0, uint16_t phase_offset = 0)
{
    uint16_t beat = beat16( beats_per_minute, timebase);
    uint16_t beatsin = (sin16( beat + phase_offset) + 32768);
    uint16_t rangewidth = highest - lowest;
    uint16_t scaledbeat = scale16( beatsin, rangewidth);
    uint16_t result = lowest + scaledbeat;
    return result;
}

/// Generates an 8-bit sine wave at a given BPM that oscillates within
/// a given range.
/// @param beats_per_minute the frequency of the wave, in decimal
/// @param lowest the lowest output value of the sine wave
/// @param highest the highest output value of the sine wave
/// @param timebase the time offset of the wave from the millis() timer
/// @param phase_offset phase offset of the wave from the current position
LIB8STATIC uint8_t beatsin8( accum88 beats_per_minute, uint8_t lowest = 0, uint8_t highest = 255,
                            uint32_t timebase = 0, uint8_t phase_offset = 0)
{
    uint8_t beat = beat8( beats_per_minute, timebase);
    uint8_t beatsin = sin8( beat + phase_offset);
    uint8_t rangewidth = highest - lowest;
    uint8_t scaledbeat = scale8( beatsin, rangewidth);
    uint8_t result = lowest + scaledbeat;
    return result;
}

/// @} BeatGenerators

#endif

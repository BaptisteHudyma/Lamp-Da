/**
 * Test for the compact BitSet implementation
 */
#include <gtest/gtest.h>
#include <cstdint>

#include "src/system/common/bitset.h"

namespace lampda::common {

// ============================================================
// Construction & initialization
// ============================================================

TEST(bitset, default_constructor_all_zero)
{
  BitSet<32> bs;
  EXPECT_TRUE(bs.none());
  EXPECT_FALSE(bs.any());
  EXPECT_EQ(bs.count(), 0u);

  for (uint16_t i = 0; i < 32; ++i)
    EXPECT_FALSE(bs.test(i));
}

TEST(bitset, constructor_with_value)
{
  BitSet<32> bs(static_cast<uint32_t>(0xFFFFFFFF));
  EXPECT_TRUE(bs.all());
  EXPECT_TRUE(bs.any());
  EXPECT_EQ(bs.count(), 32u);
}

TEST(bitset, constructor_with_zero_value)
{
  BitSet<64> bs(0u);
  EXPECT_TRUE(bs.none());
  EXPECT_EQ(bs.count(), 0u);
}

TEST(bitset, small_bitset)
{
  BitSet<8> bs;
  EXPECT_EQ(bs.size(), 8u);
  EXPECT_TRUE(bs.none());
}

TEST(bitset, large_bitset)
{
  BitSet<256> bs;
  EXPECT_EQ(bs.size(), 256u);
  EXPECT_TRUE(bs.none());
}

TEST(bitset, size_words)
{
  EXPECT_EQ(BitSet<32>::size_words(), 1u);
  EXPECT_EQ(BitSet<33>::size_words(), 2u);
  EXPECT_EQ(BitSet<64>::size_words(), 2u);
  EXPECT_EQ(BitSet<96>::size_words(), 3u);
  EXPECT_EQ(BitSet<128>::size_words(), 4u);
}

// ============================================================
// set / reset individual bits
// ============================================================

TEST(bitset, set_single_bit)
{
  BitSet<32> bs;
  bs.set(0);
  EXPECT_TRUE(bs.test(0));
  EXPECT_FALSE(bs.test(1));
  EXPECT_EQ(bs.count(), 1u);

  bs.set(15);
  EXPECT_TRUE(bs.test(15));
  EXPECT_EQ(bs.count(), 2u);

  bs.set(31);
  EXPECT_TRUE(bs.test(31));
  EXPECT_EQ(bs.count(), 3u);
}

TEST(bitset, set_all_bits)
{
  BitSet<32> bs;
  for (uint16_t i = 0; i < 32; ++i)
    bs.set(i);
  EXPECT_TRUE(bs.all());
  EXPECT_EQ(bs.count(), 32u);
}

TEST(bitset, reset_single_bit)
{
  BitSet<32> bs;
  bs.set(0);
  bs.set(1);
  bs.set(2);
  EXPECT_EQ(bs.count(), 3u);

  bs.reset(1);
  EXPECT_FALSE(bs.test(1));
  EXPECT_TRUE(bs.test(0));
  EXPECT_TRUE(bs.test(2));
  EXPECT_EQ(bs.count(), 2u);
}

TEST(bitset, reset_all_bits)
{
  BitSet<32> bs(static_cast<uint32_t>(0xFFFFFFFF));
  bs.reset();
  EXPECT_TRUE(bs.none());
  EXPECT_EQ(bs.count(), 0u);
}

TEST(bitset, set_with_bool)
{
  BitSet<16> bs;
  bs.set(0, true);
  bs.set(1, true);
  bs.set(2, false);
  EXPECT_TRUE(bs.test(0));
  EXPECT_TRUE(bs.test(1));
  EXPECT_FALSE(bs.test(2));
  EXPECT_EQ(bs.count(), 2u);
}

TEST(bitset, reset_with_bool)
{
  BitSet<16> bs;
  bs.set(0, true);
  bs.set(0, false);
  EXPECT_FALSE(bs.test(0));
  EXPECT_EQ(bs.count(), 0u);
}

// ============================================================
// Out-of-bounds handling (should be no-op)
// ============================================================

TEST(bitset, set_out_of_bounds_is_noop)
{
  BitSet<32> bs;
  bs.set(32); // exactly size
  bs.set(63); // way out
  bs.set(1000);
  EXPECT_TRUE(bs.none());
  EXPECT_EQ(bs.count(), 0u);
}

TEST(bitset, reset_out_of_bounds_is_noop)
{
  BitSet<32> bs;
  bs.set(0);
  bs.reset(32);
  EXPECT_TRUE(bs.test(0));
}

TEST(bitset, flip_out_of_bounds_is_noop)
{
  BitSet<32> bs;
  bs.flip(32);
  EXPECT_TRUE(bs.none());
}

TEST(bitset, test_out_of_bounds_returns_false)
{
  BitSet<32> bs;
  bs.set(0);
  EXPECT_FALSE(bs.test(32));
  EXPECT_FALSE(bs.test(1000));
}

TEST(bitset, operator_bracket_out_of_bounds_returns_false)
{
  BitSet<32> bs;
  EXPECT_FALSE(bs[32]);
  EXPECT_FALSE(bs[1000]);
}

// ============================================================
// flip
// ============================================================

TEST(bitset, flip_single_bit)
{
  BitSet<16> bs;
  bs.flip(5);
  EXPECT_TRUE(bs.test(5));
  EXPECT_EQ(bs.count(), 1u);

  bs.flip(5);
  EXPECT_FALSE(bs.test(5));
  EXPECT_EQ(bs.count(), 0u);
}

TEST(bitset, flip_all_bits)
{
  BitSet<32> bs;
  bs.flip();
  EXPECT_TRUE(bs.all());
  EXPECT_EQ(bs.count(), 32u);

  bs.flip();
  EXPECT_TRUE(bs.none());
  EXPECT_EQ(bs.count(), 0u);
}

// ============================================================
// operator[] proxy
// ============================================================

TEST(bitset, non_const_operator_set)
{
  BitSet<16> bs;
  bs[0] = true;
  bs[1] = true;
  bs[2] = false;
  EXPECT_TRUE(bs.test(0));
  EXPECT_TRUE(bs.test(1));
  EXPECT_FALSE(bs.test(2));
  EXPECT_EQ(bs.count(), 2u);
}

TEST(bitset, non_const_operator_clear)
{
  BitSet<16> bs;
  bs[0] = true;
  bs[1] = true;
  bs[2] = true;
  EXPECT_EQ(bs.count(), 3u);

  bs[1] = false;
  EXPECT_FALSE(bs.test(1));
  EXPECT_EQ(bs.count(), 2u);
}

TEST(bitset, non_const_operator_copy)
{
  BitSet<16> a, b;
  a[0] = true;
  a[1] = true;

  b[0] = a[0];
  b[1] = a[1];
  b[2] = a[2];

  EXPECT_TRUE(b.test(0));
  EXPECT_TRUE(b.test(1));
  EXPECT_FALSE(b.test(2));
}

TEST(bitset, non_const_operator_flip)
{
  BitSet<8> bs;
  bs[3] = true;
  bs[3].flip();
  EXPECT_FALSE(bs.test(3));

  bs[3].flip();
  EXPECT_TRUE(bs.test(3));
}

// ============================================================
// count / any / all / none
// ============================================================

TEST(bitset, count_various)
{
  BitSet<16> bs;
  EXPECT_EQ(bs.count(), 0u);

  bs.set(0);
  EXPECT_EQ(bs.count(), 1u);

  bs.set(15);
  EXPECT_EQ(bs.count(), 2u);

  for (uint16_t i = 0; i < 16; ++i)
    bs.set(i);
  EXPECT_EQ(bs.count(), 16u);
}

TEST(bitset, any_various)
{
  BitSet<16> bs;
  EXPECT_FALSE(bs.any());

  bs.set(0);
  EXPECT_TRUE(bs.any());

  bs.reset();
  EXPECT_FALSE(bs.any());
}

TEST(bitset, all_various)
{
  BitSet<16> bs;
  EXPECT_FALSE(bs.all());

  for (uint16_t i = 0; i < 16; ++i)
    bs.set(i);
  EXPECT_TRUE(bs.all());

  bs.reset(0);
  EXPECT_FALSE(bs.all());

  BitSet<31> bs2;
  for (uint16_t i = 0; i < bs2.size(); ++i)
    bs2.set(i);
  EXPECT_TRUE(bs2.all());

  bs2.reset(0);
  EXPECT_FALSE(bs2.all());

  BitSet<33> bs3;
  for (uint16_t i = 0; i < bs3.size(); ++i)
    bs3.set(i);
  EXPECT_TRUE(bs3.all());

  bs3.reset(0);
  EXPECT_FALSE(bs3.all());
}

TEST(bitset, none_various)
{
  BitSet<16> bs;
  EXPECT_TRUE(bs.none());

  bs.set(0);
  EXPECT_FALSE(bs.none());

  bs.reset();
  EXPECT_TRUE(bs.none());
}

// ============================================================
// Bitwise operators (binary)
// ============================================================

TEST(bitset, operator_and)
{
  BitSet<32> a, b;
  a.set(0);
  a.set(1);
  a.set(2);
  b.set(1);
  b.set(2);
  b.set(3);

  auto result = a & b;
  EXPECT_TRUE(result.test(1));
  EXPECT_TRUE(result.test(2));
  EXPECT_FALSE(result.test(0));
  EXPECT_FALSE(result.test(3));
  EXPECT_EQ(result.count(), 2u);
}

TEST(bitset, operator_or)
{
  BitSet<32> a, b;
  a.set(0);
  a.set(1);
  b.set(2);
  b.set(3);

  auto result = a | b;
  EXPECT_TRUE(result.test(0));
  EXPECT_TRUE(result.test(1));
  EXPECT_TRUE(result.test(2));
  EXPECT_TRUE(result.test(3));
  EXPECT_EQ(result.count(), 4u);
}

TEST(bitset, operator_minus)
{
  BitSet<32> a, b;
  a.set(0);
  a.set(1);
  a.set(2);
  b.set(1);
  b.set(2);
  b.set(3);

  auto result = a - b;
  EXPECT_TRUE(result.test(0));
  EXPECT_FALSE(result.test(1));
  EXPECT_FALSE(result.test(2));
  EXPECT_FALSE(result.test(3));
  EXPECT_EQ(result.count(), 1u);
}

TEST(bitset, operator_not)
{
  BitSet<8> bs;
  bs.set(0);
  bs.set(1);

  auto result = ~bs;
  EXPECT_FALSE(result.test(0));
  EXPECT_FALSE(result.test(1));
  for (uint16_t i = 2; i < 8; ++i)
    EXPECT_TRUE(result.test(i));
  EXPECT_EQ(result.count(), 6u);
}

// ============================================================
// In-place bitwise operators
// ============================================================

TEST(bitset, operator_and_equal)
{
  BitSet<32> a, b;
  a.set(0);
  a.set(1);
  a.set(2);
  b.set(1);
  b.set(2);
  b.set(3);

  a &= b;
  EXPECT_TRUE(a.test(1));
  EXPECT_TRUE(a.test(2));
  EXPECT_FALSE(a.test(0));
  EXPECT_EQ(a.count(), 2u);
}

TEST(bitset, operator_or_equal)
{
  BitSet<32> a, b;
  a.set(0);
  b.set(1);

  a |= b;
  EXPECT_TRUE(a.test(0));
  EXPECT_TRUE(a.test(1));
  EXPECT_EQ(a.count(), 2u);
}

TEST(bitset, operator_xor_equal)
{
  BitSet<32> a, b;
  a.set(0);
  a.set(1);
  b.set(1);
  b.set(2);

  a ^= b;
  EXPECT_TRUE(a.test(0));
  EXPECT_FALSE(a.test(1));
  EXPECT_TRUE(a.test(2));
  EXPECT_EQ(a.count(), 2u);
}

TEST(bitset, operator_not_equal)
{
  BitSet<8> bs;
  bs.set(0);
  bs.set(1);

  bs &= ~bs;
  EXPECT_TRUE(bs.none());
  EXPECT_EQ(bs.count(), 0u);

  bs.set(0);
  bs |= bs;
  EXPECT_TRUE(bs.test(0));
  EXPECT_EQ(bs.count(), 1u);

  bs.set(1);
  bs ^= bs;
  EXPECT_FALSE(bs.test(1));
  EXPECT_EQ(bs.count(), 0u);
}

// ============================================================
// Equality
// ============================================================

TEST(bitset, equality_same)
{
  BitSet<32> a, b;
  a.set(0);
  a.set(5);
  b.set(0);
  b.set(5);
  EXPECT_TRUE(a == b);
  EXPECT_FALSE(a != b);
}

TEST(bitset, equality_different)
{
  BitSet<32> a, b;
  a.set(0);
  b.set(1);
  EXPECT_FALSE(a == b);
  EXPECT_TRUE(a != b);
}

TEST(bitset, equality_all_zero)
{
  BitSet<32> a, b;
  EXPECT_TRUE(a == b);
}

TEST(bitset, equality_all_one)
{
  BitSet<32> a, b;
  a.set();
  b.set();
  EXPECT_TRUE(a == b);
}

// ============================================================
// Cross-word operations (bits spanning storage boundaries)
// ============================================================

TEST(bitset, cross_word_operations)
{
  BitSet<64> bs;

  // Set bits in both words
  bs.set(31); // last bit of first word
  bs.set(32); // first bit of second word
  bs.set(63); // last bit of second word

  EXPECT_TRUE(bs.test(31));
  EXPECT_TRUE(bs.test(32));
  EXPECT_TRUE(bs.test(63));
  EXPECT_EQ(bs.count(), 3u);

  // Clear across boundary
  bs.reset(31);
  bs.reset(32);
  EXPECT_FALSE(bs.test(31));
  EXPECT_FALSE(bs.test(32));
  EXPECT_TRUE(bs.test(63));
  EXPECT_EQ(bs.count(), 1u);

  // Flip across boundary
  bs.flip(31);
  bs.flip(32);
  EXPECT_TRUE(bs.test(31));
  EXPECT_TRUE(bs.test(32));
  EXPECT_EQ(bs.count(), 3u);
}

TEST(bitset, cross_word_bitwise_ops)
{
  BitSet<64> a, b;
  a.set(31);
  a.set(32);
  b.set(31);
  b.set(63);

  auto andResult = a & b;
  EXPECT_TRUE(andResult.test(31));
  EXPECT_FALSE(andResult.test(32));
  EXPECT_FALSE(andResult.test(63));

  auto orResult = a | b;
  EXPECT_TRUE(orResult.test(31));
  EXPECT_TRUE(orResult.test(32));
  EXPECT_TRUE(orResult.test(63));

  auto minusResult = a - b;
  EXPECT_FALSE(minusResult.test(31));
  EXPECT_TRUE(minusResult.test(32));
  EXPECT_FALSE(minusResult.test(63));
}

// ============================================================
// Multi-word bitsets
// ============================================================

TEST(bitset, multiword_set_all)
{
  BitSet<96> bs;
  bs.set();
  EXPECT_TRUE(bs.all());
  EXPECT_EQ(bs.count(), 96u);
}

TEST(bitset, multiword_set_individual)
{
  BitSet<96> bs;
  for (uint16_t i = 0; i < 96; ++i)
    bs.set(i);
  EXPECT_TRUE(bs.all());
  EXPECT_EQ(bs.count(), 96u);
}

TEST(bitset, multiword_reset_all)
{
  BitSet<96> bs;
  bs.set();
  bs.reset();
  EXPECT_TRUE(bs.none());
  EXPECT_EQ(bs.count(), 0u);
}

// ============================================================
// Edge cases
// ============================================================

TEST(bitset, single_bit_bitset)
{
  BitSet<1> bs;
  EXPECT_TRUE(bs.none());

  bs.set(0);
  EXPECT_TRUE(bs.test(0));
  EXPECT_TRUE(bs.any());
  EXPECT_EQ(bs.count(), 1u);

  bs.reset(0);
  EXPECT_TRUE(bs.none());
}

TEST(bitset, flip_single_bit_bitset)
{
  BitSet<1> bs;
  bs.flip();
  EXPECT_TRUE(bs.test(0));
  EXPECT_TRUE(bs.all());

  bs.flip();
  EXPECT_TRUE(bs.none());
}

} // namespace lampda::common

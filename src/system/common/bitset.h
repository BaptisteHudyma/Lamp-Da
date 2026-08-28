/*! \file bitset.h
    \brief Define a compact bitset class for compile-time sized bit arrays.
*/

#pragma once

#include <array>
#include <cstdint>
#include <cstddef>

namespace lampda {
namespace common {

/**
 * \brief A compact, zero-allocation bitset for compile-time sized bit arrays.
 *
 * This is a lightweight alternative to std::bitset, designed for embedded
 * environments where std::bitset may not be available or desirable.
 *
 * \tparam N The number of bits in the bitset
 */
template<uint16_t N> class BitSet
{
public:
  using storage_t = uint32_t;
  /// The underlying storage type, sized to hold at least N bits
  static constexpr uint16_t storageSize = (N + sizeof(storage_t) * 8 - 1) / (sizeof(storage_t) * 8);
  static constexpr uint16_t bitsPerStorage = sizeof(storage_t) * 8;

private:
  std::array<storage_t, storageSize> _data;

  /// Return the bit index within a storage word for bit position i
  static constexpr uint16_t word_index(uint16_t i) noexcept { return i / bitsPerStorage; }

  /// Return the bit mask for bit position i within its storage word
  static constexpr storage_t bit_mask(uint16_t i) noexcept { return static_cast<storage_t>(1) << (i % bitsPerStorage); }

  /// Generate a mask with the lower `N % bitsPerStorage` bits set
  static constexpr storage_t last_word_mask() noexcept
  {
    const uint16_t lastWordBits = N % bitsPerStorage;
    if (lastWordBits == 0)
      return static_cast<storage_t>(~storage_t {0});
    storage_t mask = static_cast<storage_t>(1) << lastWordBits;
    return mask - 1;
  }

public:
  /// Default constructor: all bits set to zero
  constexpr BitSet() : _data {} { reset(); }

  /**
   * \brief Construct with a single storage value repeated for all words.
   * \param value The value to initialize all storage words with
   */
  explicit constexpr BitSet(storage_t value) : _data {}
  {
    for (uint16_t i = 0; i < storageSize - 1; ++i)
      _data[i] = value;
    _data[storageSize - 1] = value & last_word_mask();
  }

  /// Set all bits to 1
  void set() noexcept
  {
    storage_t fullMask = static_cast<storage_t>(~storage_t {0});
    for (uint16_t i = 0; i < storageSize - 1; ++i)
      _data[i] = fullMask;
    _data[storageSize - 1] = last_word_mask();
  }

  /// Set all bits to 0
  void reset() noexcept
  {
    for (auto& word: _data)
      word = 0;
  }

  /**
   * \brief Set the bit at position pos to 1.
   * \param pos Bit position (0 to N-1)
   */
  void set(uint16_t pos) noexcept
  {
    if (pos >= N)
      return;
    _data[word_index(pos)] |= bit_mask(pos);
  }

  /**
   * \brief Set the bit at position pos to the given value.
   * \param pos Bit position (0 to N-1)
   * \param value True to set the bit, false to clear it
   */
  void set(uint16_t pos, bool value) noexcept
  {
    if (pos >= N)
      return;
    if (value)
      _data[word_index(pos)] |= bit_mask(pos);
    else
      _data[word_index(pos)] &= ~bit_mask(pos);
  }

  /**
   * \brief Clear the bit at position pos.
   * \param pos Bit position (0 to N-1)
   */
  void reset(uint16_t pos) noexcept
  {
    if (pos >= N)
      return;
    _data[word_index(pos)] &= ~bit_mask(pos);
  }

  /**
   * \brief Flip (invert) the bit at position pos.
   * \param pos Bit position (0 to N-1)
   */
  void flip(uint16_t pos) noexcept
  {
    if (pos >= N)
      return;
    _data[word_index(pos)] ^= bit_mask(pos);
  }

  /**
   * \brief Get the value of the bit at position pos.
   * \param pos Bit position (0 to N-1)
   * \return True if the bit is set, false otherwise
   */
  constexpr bool test(uint16_t pos) const noexcept
  {
    if (pos >= N)
      return false;
    return (_data[word_index(pos)] & bit_mask(pos)) != 0;
  }

  /// Alias for test(pos)
  constexpr bool operator[](uint16_t pos) const noexcept { return test(pos); }

  /// Non-const operator[]: returns a reference-like proxy that allows set/reset
  struct reference
  {
    BitSet* parent;
    uint16_t pos;

    reference& operator=(bool v) noexcept
    {
      if (pos >= parent->size())
        return *this;
      if (v)
        parent->_data[parent->word_index(pos)] |= parent->bit_mask(pos);
      else
        parent->_data[parent->word_index(pos)] &= ~parent->bit_mask(pos);
      return *this;
    }

    reference& operator=(const reference& other) noexcept
    {
      if (pos >= parent->size())
        return *this;
      bool val = other;
      if (val)
        parent->_data[parent->word_index(pos)] |= parent->bit_mask(pos);
      else
        parent->_data[parent->word_index(pos)] &= ~parent->bit_mask(pos);
      return *this;
    }

    constexpr operator bool() const noexcept
    {
      if (pos >= parent->size())
        return false;
      return (parent->_data[parent->word_index(pos)] & parent->bit_mask(pos)) != 0;
    }

    reference& flip() noexcept
    {
      if (pos >= parent->size())
        return *this;
      parent->_data[parent->word_index(pos)] ^= parent->bit_mask(pos);
      return *this;
    }
  };

  /// Non-const operator[] returning a proxy for bit manipulation
  reference operator[](uint16_t pos) noexcept { return reference {this, pos}; }

  /**
   * \brief Count the number of bits set to 1.
   * \return The population count
   */
  constexpr uint16_t count() const noexcept
  {
    uint16_t total = 0;
    for (const auto& word: _data)
    {
      storage_t w = word;
      while (w)
      {
        total += static_cast<uint16_t>(w & 1);
        w >>= 1;
      }
    }
    return total;
  }

  /// Return true if any bit is set
  constexpr bool any() const noexcept
  {
    for (const auto& word: _data)
      if (word != 0)
        return true;
    return false;
  }

  /// Return true if all bits are set
  constexpr bool all() const noexcept
  {
    storage_t fullMask = static_cast<storage_t>(~storage_t {0});
    for (uint16_t i = 0; i < storageSize - 1; ++i)
      if (_data[i] != fullMask)
        return false;
    return (_data[storageSize - 1] & last_word_mask()) == last_word_mask();
  }

  /// Return true if no bits are set
  constexpr bool none() const noexcept
  {
    for (const auto& word: _data)
      if (word != 0)
        return false;
    return true;
  }

  /// Flip all bits
  void flip() noexcept
  {
    storage_t fullMask = static_cast<storage_t>(~storage_t {0});
    for (uint16_t i = 0; i < storageSize - 1; ++i)
      _data[i] ^= fullMask;
    _data[storageSize - 1] ^= last_word_mask();
  }

  /**
   * \brief Compute the intersection of this bitset with another.
   * \param other The other bitset (must be the same size)
   * \return A new bitset with bits set where both operands have them set
   */
  constexpr BitSet<N> operator&(const BitSet<N>& other) const noexcept
  {
    BitSet<N> result;
    for (uint16_t i = 0; i < storageSize; ++i)
      result._data[i] = _data[i] & other._data[i];
    return result;
  }

  /**
   * \brief Compute the union of this bitset with another.
   * \param other The other bitset (must be the same size)
   * \return A new bitset with bits set where either operand has them set
   */
  constexpr BitSet<N> operator|(const BitSet<N>& other) const noexcept
  {
    BitSet<N> result;
    for (uint16_t i = 0; i < storageSize; ++i)
      result._data[i] = _data[i] | other._data[i];
    return result;
  }

  /**
   * \brief Compute the difference of this bitset with another.
   * \param other The other bitset (must be the same size)
   * \return A new bitset with bits set where this has them but other does not
   */
  constexpr BitSet<N> operator-(const BitSet<N>& other) const noexcept
  {
    BitSet<N> result;
    for (uint16_t i = 0; i < storageSize; ++i)
      result._data[i] = _data[i] & ~other._data[i];
    return result;
  }

  /**
   * \brief Compute the bitwise NOT of this bitset.
   * \return A new bitset with all bits flipped
   */
  BitSet<N> operator~() const noexcept
  {
    BitSet<N> result;
    storage_t fullMask = static_cast<storage_t>(~storage_t {0});
    for (uint16_t i = 0; i < storageSize - 1; ++i)
      result._data[i] = ~_data[i];
    result._data[storageSize - 1] = ~_data[storageSize - 1] & last_word_mask(); // <-- use helper
    return result;
  }

  /// In-place bitwise AND
  BitSet<N>& operator&=(const BitSet<N>& other) noexcept
  {
    for (uint16_t i = 0; i < storageSize; ++i)
      _data[i] &= other._data[i];
    return *this;
  }

  /// In-place bitwise OR
  BitSet<N>& operator|=(const BitSet<N>& other) noexcept
  {
    for (uint16_t i = 0; i < storageSize; ++i)
      _data[i] |= other._data[i];
    return *this;
  }

  /// In-place bitwise XOR
  BitSet<N>& operator^=(const BitSet<N>& other) noexcept
  {
    for (uint16_t i = 0; i < storageSize; ++i)
      _data[i] ^= other._data[i];
    return *this;
  }

  /// Check equality with another bitset
  constexpr bool operator==(const BitSet<N>& other) const noexcept
  {
    for (uint16_t i = 0; i < storageSize; ++i)
      if (_data[i] != other._data[i])
        return false;
    return true;
  }

  /// Check inequality with another bitset
  constexpr bool operator!=(const BitSet<N>& other) const noexcept { return not operator==(other); }

  /// Return the number of bits this bitset can hold
  static constexpr uint16_t size() noexcept { return N; }

  /// Return the number of storage words used
  static constexpr uint16_t size_words() noexcept { return storageSize; }
};

} // namespace common
} // namespace lampda

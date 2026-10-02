#pragma once

#include <cstddef>
#include <iterator>

namespace lampda {
namespace common {

/**
 * static_vector: Fixed-capacity vector with no dynamic allocation.
 * Throws on overflow (can be customized to return error codes instead).
 */
template<typename T, std::size_t Capacity> class static_vector
{
#ifdef LMBD_CPP17
#define CST_CONSTREXPR constexpr
#else
#define CST_CONSTREXPR
#endif

public:
  using value_type = T;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using reference = T&;
  using const_reference = const T&;
  using pointer = T*;
  using const_pointer = const T*;
  using iterator = T*;
  using const_iterator = const T*;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  // Constructors
  constexpr static_vector() noexcept : m_size(0) {}

  CST_CONSTREXPR explicit static_vector(size_type count, const T& value = T()) noexcept : m_size(0)
  {
    if (count > Capacity)
      return; // Silently truncate instead of throwing
    for (size_type i = 0; i < count; ++i)
    {
      m_data[i] = value;
    }
    m_size = count;
  }

  CST_CONSTREXPR static_vector(const static_vector& other) noexcept : m_size(other.m_size)
  {
    std::copy(other.begin(), other.end(), m_data);
  }

  CST_CONSTREXPR static_vector& operator=(const static_vector& other) noexcept
  {
    if (this != &other)
    {
      m_size = other.m_size;
      std::copy(other.begin(), other.end(), m_data);
    }
    return *this;
  }

  // Element access
  CST_CONSTREXPR reference operator[](size_type pos) noexcept { return m_data[pos]; }

  CST_CONSTREXPR const_reference operator[](size_type pos) const noexcept { return m_data[pos]; }

  CST_CONSTREXPR pointer data() noexcept { return m_data; }
  CST_CONSTREXPR const_pointer data() const noexcept { return m_data; }

  // Iterators
  CST_CONSTREXPR iterator begin() noexcept { return m_data; }
  CST_CONSTREXPR const_iterator begin() const noexcept { return m_data; }
  CST_CONSTREXPR const_iterator cbegin() const noexcept { return m_data; }

  CST_CONSTREXPR iterator end() noexcept { return m_data + m_size; }
  CST_CONSTREXPR const_iterator end() const noexcept { return m_data + m_size; }
  CST_CONSTREXPR const_iterator cend() const noexcept { return m_data + m_size; }

  CST_CONSTREXPR reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
  CST_CONSTREXPR const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }

  CST_CONSTREXPR reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
  CST_CONSTREXPR const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }

  // Size and capacity
  constexpr size_type size() const noexcept { return m_size; }
  constexpr size_type max_size() const noexcept { return Capacity; }
  constexpr bool empty() const noexcept { return m_size == 0; }

  // Modifiers
  CST_CONSTREXPR void push_back(const T& value) noexcept
  {
    if (m_size < Capacity)
    {
      m_data[m_size++] = value;
    }
  }

  CST_CONSTREXPR void pop_back() noexcept
  {
    if (m_size > 0)
    {
      --m_size;
    }
  }

  constexpr void clear() noexcept { m_size = 0; }

  CST_CONSTREXPR iterator erase(const_iterator pos) noexcept
  {
    auto it = begin() + (pos - cbegin());
    std::move(it + 1, end(), it);
    --m_size;
    return it;
  }

  CST_CONSTREXPR iterator erase(const_iterator first, const_iterator last) noexcept
  {
    auto it = begin() + (first - cbegin());
    auto count = last - first;
    std::move(last, cend(), it);
    m_size -= count;
    return it;
  }

  CST_CONSTREXPR iterator insert(const_iterator pos, const T& value) noexcept
  {
    if (m_size >= Capacity)
      return end();
    auto it = begin() + (pos - cbegin());
    std::move_backward(it, end(), end() + 1);
    *it = value;
    ++m_size;
    return it;
  }

private:
  T m_data[Capacity] {};
  size_type m_size = 0;
};

} // namespace common
} // namespace lampda

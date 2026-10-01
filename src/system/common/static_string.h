#pragma once

#include <cstring>
#include <algorithm>
#include <stdexcept>
#include <type_traits>
#include <iterator>
#include <iosfwd>

#include "src/system/common/static_vector.h"

namespace lampda {
namespace common {

/**
 * static_string: Fixed-capacity string with no dynamic allocation, no exceptions.
 * Template parameter N defines the maximum capacity (excluding null terminator).
 */
template<std::size_t N> class static_string
{
#ifdef LMBD_CPP17
#define CST_CONSTREXPR constexpr
#else
#define CST_CONSTREXPR
#endif

public:
  using value_type = char;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using reference = char&;
  using const_reference = const char&;
  using pointer = char*;
  using const_pointer = const char*;
  using iterator = char*;
  using const_iterator = const char*;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  // Constructors
  CST_CONSTREXPR static_string() noexcept : m_size(0) { m_data[0] = '\0'; }

  CST_CONSTREXPR static_string(const char* str) noexcept : m_size(0)
  {
    if (str)
    {
      while (m_size < N && str[m_size] != '\0')
      {
        m_data[m_size] = str[m_size];
        ++m_size;
      }
    }
    m_data[m_size] = '\0';
  }

  CST_CONSTREXPR static_string(const char* str, size_type count) noexcept : m_size(0)
  {
    if (str)
    {
      size_type len = std::min(count, N);
      std::copy(str, str + len, m_data);
      m_size = len;
    }
    m_data[m_size] = '\0';
  }

  template<std::size_t M> CST_CONSTREXPR static_string(const static_string<M>& other) noexcept
  {
    m_size = std::min(other.size(), N);
    std::copy(other.begin(), other.begin() + m_size, m_data);
    m_data[m_size] = '\0';
  }

  CST_CONSTREXPR static_string(const static_string& other) noexcept : m_size(other.m_size)
  {
    std::copy(other.m_data, other.m_data + m_size, m_data);
    m_data[m_size] = '\0';
  }

  CST_CONSTREXPR static_string& operator=(const static_string& other) noexcept
  {
    if (this != &other)
    {
      m_size = other.m_size;
      std::copy(other.m_data, other.m_data + m_size, m_data);
      m_data[m_size] = '\0';
    }
    return *this;
  }

  CST_CONSTREXPR static_string& operator=(const char* str) noexcept
  {
    m_size = 0;
    if (str)
    {
      while (m_size < N && str[m_size] != '\0')
      {
        m_data[m_size] = str[m_size];
        ++m_size;
      }
    }
    m_data[m_size] = '\0';
    return *this;
  }

  // Element access
  CST_CONSTREXPR reference operator[](size_type pos) noexcept { return m_data[pos]; }
  CST_CONSTREXPR const_reference operator[](size_type pos) const noexcept { return m_data[pos]; }

  CST_CONSTREXPR reference front() noexcept { return m_data[0]; }
  CST_CONSTREXPR const_reference front() const noexcept { return m_data[0]; }

  CST_CONSTREXPR reference back() noexcept { return m_data[m_size - 1]; }
  CST_CONSTREXPR const_reference back() const noexcept { return m_data[m_size - 1]; }

  CST_CONSTREXPR const_pointer data() const noexcept { return m_data; }
  CST_CONSTREXPR pointer data() noexcept { return m_data; }

  CST_CONSTREXPR const_pointer c_str() const noexcept { return m_data; }

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
  constexpr size_type length() const noexcept { return m_size; }
  constexpr size_type max_size() const noexcept { return N; }
  constexpr size_type capacity() const noexcept { return N; }
  constexpr bool empty() const noexcept { return m_size == 0; }

  // Modifiers
  CST_CONSTREXPR void clear() noexcept
  {
    m_size = 0;
    m_data[0] = '\0';
  }

  CST_CONSTREXPR void push_back(char c) noexcept
  {
    if (m_size < N)
    {
      m_data[m_size++] = c;
      m_data[m_size] = '\0';
    }
  }

  CST_CONSTREXPR void pop_back() noexcept
  {
    if (m_size > 0)
    {
      --m_size;
      m_data[m_size] = '\0';
    }
  }

  CST_CONSTREXPR static_string& append(const char* str) noexcept
  {
    if (str)
    {
      while (m_size < N && *str != '\0')
      {
        m_data[m_size++] = *str++;
      }
      m_data[m_size] = '\0';
    }
    return *this;
  }

  CST_CONSTREXPR static_string& append(const char* str, size_type count) noexcept
  {
    if (str)
    {
      size_type len = std::min(count, N - m_size);
      std::copy(str, str + len, m_data + m_size);
      m_size += len;
      m_data[m_size] = '\0';
    }
    return *this;
  }

  template<std::size_t M> CST_CONSTREXPR static_string& append(const static_string<M>& other) noexcept
  {
    return append(other.data(), other.size());
  }

  CST_CONSTREXPR static_string& append(const static_string& other) noexcept
  {
    return append(other.data(), other.size());
  }

  CST_CONSTREXPR static_string& operator+=(const char* str) noexcept { return append(str); }

  CST_CONSTREXPR static_string& operator+=(char c) noexcept
  {
    push_back(c);
    return *this;
  }

  template<std::size_t M> CST_CONSTREXPR static_string& operator+=(const static_string<M>& other) noexcept
  {
    return append(other);
  }

  // Substring
  CST_CONSTREXPR static_string substr(size_type pos = 0, size_type count = -1) const noexcept
  {
    count = std::min(count, m_size - pos);
    return static_string(m_data + pos, count);
  }

  // Search
  CST_CONSTREXPR size_type find(char c, size_type pos = 0) const noexcept
  {
    for (size_type i = pos; i < m_size; ++i)
    {
      if (m_data[i] == c)
        return i;
    }
    return npos;
  }

  CST_CONSTREXPR size_type find(const char* str, size_type pos = 0) const noexcept
  {
    if (!str || *str == '\0')
      return pos;
    size_type len = 0;
    while (str[len] != '\0')
      ++len;

    for (size_type i = pos; i <= m_size - len; ++i)
    {
      if (std::equal(m_data + i, m_data + i + len, str))
      {
        return i;
      }
    }
    return npos;
  }

  CST_CONSTREXPR size_type rfind(char c, size_type pos = npos) const noexcept
  {
    if (m_size == 0)
      return npos;
    pos = std::min(pos, m_size - 1);
    for (size_type i = pos; i != static_cast<size_type>(-1); --i)
    {
      if (m_data[i] == c)
        return i;
    }
    return npos;
  }

  // Comparison
  CST_CONSTREXPR int compare(const static_string& other) const noexcept { return compare(other.data()); }

  CST_CONSTREXPR int compare(const char* str) const noexcept
  {
    size_type i = 0;
    while (i < m_size && str[i] != '\0')
    {
      if (m_data[i] != str[i])
      {
        return static_cast<unsigned char>(m_data[i]) - static_cast<unsigned char>(str[i]);
      }
      ++i;
    }
    if (i < m_size)
      return 1;
    if (str[i] != '\0')
      return -1;
    return 0;
  }

  CST_CONSTREXPR bool operator==(const static_string& other) const noexcept { return compare(other) == 0; }

  CST_CONSTREXPR bool operator==(const char* str) const noexcept { return compare(str) == 0; }

  CST_CONSTREXPR bool operator!=(const static_string& other) const noexcept { return compare(other) != 0; }

  CST_CONSTREXPR bool operator!=(const char* str) const noexcept { return compare(str) != 0; }

  CST_CONSTREXPR bool operator<(const static_string& other) const noexcept { return compare(other) < 0; }

  CST_CONSTREXPR bool operator<(const char* str) const noexcept { return compare(str) < 0; }

  CST_CONSTREXPR bool operator<=(const static_string& other) const noexcept { return compare(other) <= 0; }

  CST_CONSTREXPR bool operator<=(const char* str) const noexcept { return compare(str) <= 0; }

  CST_CONSTREXPR bool operator>(const static_string& other) const noexcept { return compare(other) > 0; }

  CST_CONSTREXPR bool operator>(const char* str) const noexcept { return compare(str) > 0; }

  CST_CONSTREXPR bool operator>=(const static_string& other) const noexcept { return compare(other) >= 0; }

  CST_CONSTREXPR bool operator>=(const char* str) const noexcept { return compare(str) >= 0; }

  // Erase and replace
  CST_CONSTREXPR iterator erase(const_iterator pos) noexcept
  {
    auto it = begin() + (pos - cbegin());
    std::move(it + 1, end(), it);
    --m_size;
    m_data[m_size] = '\0';
    return it;
  }

  CST_CONSTREXPR iterator erase(const_iterator first, const_iterator last) noexcept
  {
    auto it = begin() + (first - cbegin());
    auto count = last - first;
    std::move(last, cend(), it);
    m_size -= count;
    m_data[m_size] = '\0';
    return it;
  }

  CST_CONSTREXPR static_string& erase(size_type pos = 0, size_type count = npos) noexcept
  {
    if (pos >= m_size)
      return *this;
    count = std::min(count, m_size - pos);
    std::move(m_data + pos + count, m_data + m_size, m_data + pos);
    m_size -= count;
    m_data[m_size] = '\0';
    return *this;
  }

  CST_CONSTREXPR iterator insert(const_iterator pos, char c) noexcept
  {
    if (m_size >= N)
      return end();
    auto it = begin() + (pos - cbegin());
    std::move_backward(it, end(), end() + 1);
    *it = c;
    ++m_size;
    m_data[m_size] = '\0';
    return it;
  }

  CST_CONSTREXPR static_string& insert(size_type pos, const char* str) noexcept
  {
    if (!str || pos > m_size)
      return *this;
    size_type len = 0;
    while (str[len] != '\0' && m_size + len < N)
      ++len;

    if (m_size + len > N)
      len = N - m_size;

    std::move_backward(m_data + pos, m_data + m_size, m_data + m_size + len);
    std::copy(str, str + len, m_data + pos);
    m_size += len;
    m_data[m_size] = '\0';
    return *this;
  }

  // Replace
  CST_CONSTREXPR static_string& replace(size_type pos, size_type count, const char* str) noexcept
  {
    if (pos >= m_size)
      return *this;
    erase(pos, count);
    insert(pos, str);
    return *this;
  }

  // Starts with / Ends with
  CST_CONSTREXPR bool starts_with(const char* prefix) const noexcept
  {
    size_type len = 0;
    while (prefix[len] != '\0')
      ++len;
    if (len > m_size)
      return false;
    return std::equal(prefix, prefix + len, m_data);
  }

  CST_CONSTREXPR bool starts_with(char c) const noexcept { return m_size > 0 && m_data[0] == c; }

  CST_CONSTREXPR bool ends_with(const char* suffix) const noexcept
  {
    size_type len = 0;
    while (suffix[len] != '\0')
      ++len;
    if (len > m_size)
      return false;
    return std::equal(suffix, suffix + len, m_data + m_size - len);
  }

  constexpr bool ends_with(char c) const noexcept { return m_size > 0 && m_data[m_size - 1] == c; }

  // Check if capacity exceeded (graceful overflow detection)
  constexpr bool is_full() const noexcept { return m_size >= N; }

  // Special member for checking if the last operation failed (capacity exceeded)
  static constexpr size_type npos = static_cast<size_type>(-1);

private:
  char m_data[N + 1] {}; // +1 for null terminator
  size_type m_size = 0;
};

#ifdef LMBD_CPP17
// Deduction guide
template<std::size_t N> static_string(const char (&)[N]) -> static_string<N - 1>;
#endif

// Non-member operator+
template<std::size_t N, std::size_t M>
CST_CONSTREXPR static_string<N + M> operator+(const static_string<N>& lhs, const static_string<M>& rhs) noexcept
{
  static_string<N + M> result(lhs);
  result.append(rhs);
  return result;
}

template<std::size_t N> CST_CONSTREXPR static_string<N> operator+(const static_string<N>& lhs, const char* rhs) noexcept
{
  static_string<N> result(lhs);
  result.append(rhs);
  return result;
}

// Stream output
template<std::size_t N> std::ostream& operator<<(std::ostream& os, const static_string<N>& str)
{
  return os << str.c_str();
}

} // namespace common
} // namespace lampda

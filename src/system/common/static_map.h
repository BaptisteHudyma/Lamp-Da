/**
 * \file static_map.h
    Implementation of a static_map by Claude Haiku 4.5, corrected by Baptiste Hudyma
    The static_map behaves like the std::map but with static memory
 */

#pragma once

#include <cstddef>
#include <utility>
#include <iterator>
#include <algorithm>
#include <new>

namespace lampda {
namespace common {

/**
 * @brief A fixed-capacity associative container similar to std::map
 *        with no dynamic memory allocations and no exceptions.
 *
 * @tparam Key       The key type
 * @tparam Value     The value type
 * @tparam Capacity  Maximum number of key-value pairs
 */
template<typename Key, typename Value, std::size_t Capacity> class static_map
{
public:
  using key_type = Key;
  using mapped_type = Value;
  using value_type = std::pair<const Key, Value>;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using reference = value_type&;
  using const_reference = const value_type&;
  using pointer = value_type*;
  using const_pointer = const value_type*;

private:
  // Storage for value_type with occupied flag
  struct alignas(value_type) NodeStorage
  {
    unsigned char data[sizeof(value_type)];
    bool occupied;

    NodeStorage() : occupied(false) {}
  };

  NodeStorage storage[Capacity];
  size_type count_;

  // Cast storage to value_type pointer
  value_type* get_value_ptr(NodeStorage& node) { return reinterpret_cast<value_type*>(&node.data[0]); }

  const value_type* get_value_ptr(const NodeStorage& node) const
  {
    return reinterpret_cast<const value_type*>(&node.data[0]);
  }

  // Helper to find node by key
  NodeStorage* find_node(const Key& key)
  {
    for (size_type i = 0; i < count_; ++i)
    {
      if (storage[i].occupied && get_value_ptr(storage[i])->first == key)
      {
        return &storage[i];
      }
    }
    return nullptr;
  }

  const NodeStorage* find_node(const Key& key) const
  {
    for (size_type i = 0; i < count_; ++i)
    {
      if (storage[i].occupied && get_value_ptr(storage[i])->first == key)
      {
        return &storage[i];
      }
    }
    return nullptr;
  }

public:
  // ========== Iterators ==========

  class iterator
  {
  public:
    using difference_type = std::ptrdiff_t;
    using value_type = static_map::value_type;
    using pointer = static_map::pointer;
    using reference = static_map::reference;
    using iterator_category = std::bidirectional_iterator_tag;

    iterator() : node_(nullptr), parent_(nullptr) {}

    reference operator*() const { return *reinterpret_cast<value_type*>(&node_->data[0]); }

    pointer operator->() const { return reinterpret_cast<value_type*>(&node_->data[0]); }

    iterator& operator++()
    {
      ++node_;
      skip_unoccupied();
      return *this;
    }

    iterator operator++(int)
    {
      iterator tmp = *this;
      ++(*this);
      return tmp;
    }

    iterator& operator--()
    {
      --node_;
      skip_unoccupied_reverse();
      return *this;
    }

    iterator operator--(int)
    {
      iterator tmp = *this;
      --(*this);
      return tmp;
    }

    bool operator==(const iterator& other) const { return node_ == other.node_; }

    bool operator!=(const iterator& other) const { return node_ != other.node_; }

  private:
    friend class static_map;
    NodeStorage* node_;
    static_map* parent_;

    iterator(NodeStorage* node, static_map* parent) : node_(node), parent_(parent) {}

    void skip_unoccupied()
    {
      while (node_ != &parent_->storage[Capacity] && !node_->occupied)
      {
        ++node_;
      }
    }

    void skip_unoccupied_reverse()
    {
      while (node_ != parent_->storage - 1 && !node_->occupied)
      {
        --node_;
      }
    }
  };

  class const_iterator
  {
  public:
    using difference_type = std::ptrdiff_t;
    using value_type = static_map::value_type;
    using pointer = static_map::const_pointer;
    using reference = static_map::const_reference;
    using iterator_category = std::bidirectional_iterator_tag;

    const_iterator() : node_(nullptr), parent_(nullptr) {}

    const_iterator(const iterator& it) : node_(it.node_), parent_(it.parent_) {}

    reference operator*() const { return *reinterpret_cast<const value_type*>(&node_->data[0]); }

    pointer operator->() const { return reinterpret_cast<const value_type*>(&node_->data[0]); }

    const_iterator& operator++()
    {
      ++node_;
      skip_unoccupied();
      return *this;
    }

    const_iterator operator++(int)
    {
      const_iterator tmp = *this;
      ++(*this);
      return tmp;
    }

    const_iterator& operator--()
    {
      --node_;
      skip_unoccupied_reverse();
      return *this;
    }

    const_iterator operator--(int)
    {
      const_iterator tmp = *this;
      --(*this);
      return tmp;
    }

    bool operator==(const const_iterator& other) const { return node_ == other.node_; }

    bool operator!=(const const_iterator& other) const { return node_ != other.node_; }

  private:
    friend class static_map;
    const NodeStorage* node_;
    const static_map* parent_;

    const_iterator(const NodeStorage* node, const static_map* parent) : node_(node), parent_(parent) {}

    void skip_unoccupied()
    {
      while (node_ != &parent_->storage[Capacity] && !node_->occupied)
      {
        ++node_;
      }
    }

    void skip_unoccupied_reverse()
    {
      while (node_ != parent_->storage - 1 && !node_->occupied)
      {
        --node_;
      }
    }
  };

  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  // ========== Constructors ==========

  static_map() : count_(0) {}

  ~static_map() { clear(); }

  // Deleted copy/move to avoid issues with const keys
  static_map(const static_map&) = delete;
  static_map& operator=(const static_map&) = delete;
  static_map(static_map&&) = delete;
  static_map& operator=(static_map&&) = delete;

  // ========== Element Access ==========

  /**
   * @brief Find value by key
   * @return Iterator to element, or end() if not found
   */
  iterator find(const Key& key)
  {
    NodeStorage* node = find_node(key);
    if (node)
    {
      return iterator(node, this);
    }
    return end();
  }

  const_iterator find(const Key& key) const
  {
    const NodeStorage* node = find_node(key);
    if (node)
    {
      return const_iterator(node, this);
    }
    return end();
  }

  /**
   * @brief Access mapped value by key (const version)
   * @return Reference to value if key exists
   * @note No bounds checking; behavior is undefined if key doesn't exist
   */
  const Value& at(const Key& key) const
  {
    const NodeStorage* node = find_node(key);
    return get_value_ptr(*node)->second;
  }

  /**
   * @brief Access mapped value by key
   * @return Reference to value if key exists
   * @note No bounds checking; behavior is undefined if key doesn't exist
   */
  Value& at(const Key& key)
  {
    NodeStorage* node = find_node(key);
    return get_value_ptr(*node)->second;
  }

  /**
   * @brief Check if key exists
   */
  bool contains(const Key& key) const { return find_node(key) != nullptr; }

  /**
   * @brief Count occurrences of key (0 or 1)
   */
  size_type count(const Key& key) const { return contains(key) ? 1 : 0; }

  // ========== Insertion ==========

  /**
   * @brief Insert a key-value pair
   * @return Pair of (iterator, bool) where bool indicates if insertion succeeded
   * @note Returns false if key already exists or capacity is full
   */
  std::pair<iterator, bool> insert(const value_type& value)
  {
    const Key& key = value.first;

    // Check if key already exists
    NodeStorage* existing = find_node(key);
    if (existing)
    {
      return {iterator(existing, this), false};
    }

    // Check if we have space
    if (count_ >= Capacity)
    {
      return {end(), false};
    }

    // Construct in place using placement new
    new (get_value_ptr(storage[count_])) value_type(value);
    storage[count_].occupied = true;
    return {iterator(&storage[count_++], this), true};
  }

  /**
   * @brief Insert a key-value pair (rvalue reference version)
   * @return Pair of (iterator, bool) where bool indicates if insertion succeeded
   */
  std::pair<iterator, bool> insert(value_type&& value)
  {
    const Key& key = value.first;

    // Check if key already exists
    NodeStorage* existing = find_node(key);
    if (existing)
    {
      return {iterator(existing, this), false};
    }

    // Check if we have space
    if (count_ >= Capacity)
    {
      return {end(), false};
    }

    // Construct in place using placement new
    new (get_value_ptr(storage[count_])) value_type(std::move(value));
    storage[count_].occupied = true;
    return {iterator(&storage[count_++], this), true};
  }

  /**
   * @brief Emplace a new key-value pair
   * @return Pair of (iterator, bool) where bool indicates if insertion succeeded
   */
  template<typename... Args> std::pair<iterator, bool> emplace(Args&&... args)
  {
    if (count_ >= Capacity)
    {
      return {end(), false};
    }

    // Construct temporary to extract key
    value_type temp(std::forward<Args>(args)...);
    const Key& key = temp.first;

    // Check if key already exists
    NodeStorage* existing = find_node(key);
    if (existing)
    {
      return {iterator(existing, this), false};
    }

    // Construct in place using placement new
    new (get_value_ptr(storage[count_])) value_type(std::move(temp));
    storage[count_].occupied = true;
    return {iterator(&storage[count_++], this), true};
  }

  /**
   * @brief Insert or assign value
   * @return Pair of (iterator, bool) where bool indicates if insertion occurred (vs assignment)
   */
  std::pair<iterator, bool> insert_or_assign(const Key& key, const Value& value)
  {
    NodeStorage* existing = find_node(key);
    if (existing)
    {
      get_value_ptr(*existing)->second = value;
      return {iterator(existing, this), false};
    }

    if (count_ >= Capacity)
    {
      return {end(), false};
    }

    // Construct in place
    new (get_value_ptr(storage[count_])) value_type(key, value);
    storage[count_].occupied = true;
    return {iterator(&storage[count_++], this), true};
  }

  /**
   * @brief Insert or assign value (rvalue reference version)
   */
  std::pair<iterator, bool> insert_or_assign(const Key& key, Value&& value)
  {
    NodeStorage* existing = find_node(key);
    if (existing)
    {
      get_value_ptr(*existing)->second = std::move(value);
      return {iterator(existing, this), false};
    }

    if (count_ >= Capacity)
    {
      return {end(), false};
    }

    // Construct in place
    new (get_value_ptr(storage[count_])) value_type(key, std::move(value));
    storage[count_].occupied = true;
    return {iterator(&storage[count_++], this), true};
  }

  // ========== Removal ==========

  /**
   * @brief Erase element by iterator
   * @return Iterator to the element following the erased element
   */
  iterator erase(iterator pos)
  {
    NodeStorage* node = pos.node_;
    if (node && node->occupied)
    {
      // If this is not the last occupied element, swap with last
      if (count_ > 0 && node != &storage[count_ - 1])
      {
        // Find the actual last occupied node
        size_type last_idx = count_ - 1;

        // Copy the last element to this position
        new (get_value_ptr(*node)) value_type(*get_value_ptr(storage[last_idx]));
        get_value_ptr(storage[last_idx])->~value_type();
        storage[last_idx].occupied = false;
      }
      else
      {
        // Destruct the object in place
        get_value_ptr(*node)->~value_type();
        node->occupied = false;
      }

      --count_;
    }
    return ++pos;
  }

  /**
   * @brief Erase element by key
   * @return Number of elements erased (0 or 1)
   */
  size_type erase(const Key& key)
  {
    NodeStorage* node = find_node(key);
    if (node)
    {
      erase(iterator(node, this));
      return 1;
    }
    return 0;
  }

  /**
   * @brief Clear all elements
   */
  void clear()
  {
    for (size_type i = 0; i < Capacity; ++i)
    {
      if (storage[i].occupied)
      {
        get_value_ptr(storage[i])->~value_type();
        storage[i].occupied = false;
      }
    }
    count_ = 0;
  }

  // ========== Capacity ==========

  /**
   * @brief Return current number of elements
   */
  size_type size() const { return count_; }

  /**
   * @brief Return maximum capacity
   */
  constexpr size_type capacity() const { return Capacity; }

  /**
   * @brief Check if empty
   */
  bool empty() const { return count_ == 0; }

  /**
   * @brief Check if full
   */
  bool full() const { return count_ >= Capacity; }

  // ========== Iterators ==========

  iterator begin()
  {
    if (count_ == 0)
      return end();
    iterator it(&storage[0], this);
    it.skip_unoccupied();
    return it;
  }

  const_iterator begin() const
  {
    if (count_ == 0)
      return end();
    const_iterator it(&storage[0], this);
    it.skip_unoccupied();
    return it;
  }

  const_iterator cbegin() const { return begin(); }

  iterator end() { return iterator(&storage[Capacity], this); }

  const_iterator end() const { return const_iterator(&storage[Capacity], this); }

  const_iterator cend() const { return end(); }

  reverse_iterator rbegin() { return reverse_iterator(end()); }

  const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }

  const_reverse_iterator crbegin() const { return rbegin(); }

  reverse_iterator rend() { return reverse_iterator(begin()); }

  const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }

  const_reverse_iterator crend() const { return rend(); }
};

} // namespace common
} // namespace lampda

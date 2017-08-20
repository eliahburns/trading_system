//
// Created by eli on 8/4/17.
// For use with single producer, single consumer communication

#ifndef CIRCULAR_BUFFER_ALIGNED_CIRCULAR_BUFFER_HPP
#define CIRCULAR_BUFFER_ALIGNED_CIRCULAR_BUFFER_HPP

#include <type_traits>
#include <new>
#include <utility>
#include <cstdint>
#include <cassert>
#include <atomic>
#include "ipc_messages.hpp"


template<class T, std::size_t N>
class aligned_circular_buffer
// requires default constructor for type T
{
public:
  using aligned_size_type = std::uint64_t;
  using aligned_storage_type =
    typename std::aligned_storage<sizeof(T), alignof(T)>::type;

  aligned_circular_buffer()
    : m_size_{0}, read_{0}, write_{0}, capacity_{N}
    {
      assert((!(N & (N-1))) && N);

      while (m_size_ < N)
        emplace_back(T());
    }

  // an aligned buffer should not be copied or moved--only referenced
  aligned_circular_buffer(aligned_circular_buffer&) = delete;
  aligned_circular_buffer& operator=(aligned_circular_buffer&) = delete;
  aligned_circular_buffer(aligned_circular_buffer&&) = delete;
  aligned_circular_buffer& operator=(aligned_circular_buffer&&) = delete;

  ~aligned_circular_buffer()
  {
    for(std::size_t pos = 0; pos < m_size_; ++pos)
    {
      reinterpret_cast<T*>(data_+pos)->~T();
    }
  }

  const aligned_size_type capacity() const { return capacity_; }
  void clear() { read_ = write_ = 0; }
  aligned_size_type size() const { return write_ - read_; }
  const bool empty() const { return read_ == write_; }
  const bool full() const { return size() == capacity_; }

  T pop_front()
  {
    return operator[](mask(++read_));
  }

  void push_back(T elem)
  {
    if (full())
      ++read_;

    operator[](mask(++write_)) = elem;
  }

private:
  template<typename ...Args>
  void emplace_back(Args&&... args)
  {
    if( m_size_ >= N )
      throw std::bad_alloc{};

    new(data_+m_size_) T(std::forward<Args>(args)...);
    ++m_size_;
  }

  T& operator[](std::size_t pos)
  {
    return *reinterpret_cast<T*>(data_+pos);
  }

  const aligned_size_type mask(aligned_size_type val) const
  { return val & (capacity_ - 1); }

  aligned_storage_type data_[N]; // aligned uninitialized storage for N T's
  std::atomic<std::size_t> m_size_;
  std::atomic<aligned_size_type> read_;
  std::atomic<aligned_size_type> write_;
  std::atomic<aligned_size_type> capacity_;
};



#endif //CIRCULAR_BUFFER_ALIGNED_CIRCULAR_BUFFER_HPP

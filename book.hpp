//
// Created by eli on 8/13/17.
//
#ifndef TRADING_SYSTEM_COMPONENTS_BOOK_HPP
#define TRADING_SYSTEM_COMPONENTS_BOOK_HPP

#include <list>
#include <map>
#include <iostream>
#include "ipc_messages.hpp"
#include "quote.hpp"



template <typename T, typename comparator_T = std::less<aligned::aligned_t>>
class book
{
  static_assert(std::is_same<T, bid>::value || std::is_same<T, ask>::value,
                "book requires bid or ask type.");
public:
  using book_type = std::map<aligned::aligned_t, std::list<T>,
  comparator_T>;

  using book_t = std::uint64_t;

  book()
    : book_()
  {}

  void add_quote(aligned::message_t& msg)
  // make sure the order id is tracked in book_builder
  {
    ++num_quotes_;
    T to_add(msg);
    to_add.order_type = aligned::order_type::limit;
    auto it = book_.find(msg.price);
    if (it == book_.end())
    {
      std::list<T> p_list{};
      p_list.push_back(to_add);
      book_.emplace(msg.price, p_list);
      //book_[msg.price] = p_list;
    }
    else it->second.push_back(to_add);
  }

  const book_t quote_count() const { return num_quotes_; }

  T find_quote(book_t price, book_t id)
  // Returns a copy of quote if its in book, otherwise returns a blank copy
  // of a quote (zero initialized)--really only for testing.
  {
    auto book_it = book_.find(price);
    if (book_it == book_.end())
      return T();

    auto& ref_it = book_it->second;
    for (auto elem : ref_it)
      if (elem.order_id == id)
        return elem;
    return T();
  }

  void delete_quote(aligned::message_t& msg, book_t price)
  // Deleting quote requires order_id, order_type (delete_) and side.
  // If quotes are found at price level, then try to remove quote.
  {
    --num_quotes_;
    T find_quote{};
    find_quote.order_id = msg.order_id;
    find_quote.venue = msg.venue;
    auto px_list = book_.find(price);
    if (px_list != book_.end())
      if (px_list->second.size() > 0)
        px_list->second.remove(find_quote);
  }

  void modify_quote(aligned::message_t& msg, book_t price)
  {
    std::list<T>& price_list = book_.find(price)->second;
    auto it = price_list.begin();
    for( ; it != price_list.end(); ++it)
      if (it->order_id == msg.order_id)
      {
        if (it->quantity >= msg.quantity)
        {
          // quote keeps its priority
          it->quantity = msg.quantity;
          return;
        }
        // else quote loses its priority
        price_list.erase(it);
        T modified(msg);
        modified.order_type = aligned::order_type::limit;
        price_list.push_back(modified);
        return;
      }
  }

  void modify_or_delete_quote(aligned::message_t& msg, book_t price)
  {
    if (msg.type == aligned::order_type::delete_)
      delete_quote(msg, price);
    else
      modify_quote(msg, price);
  }

  book_t best_price()
  // Return best price if found, else return ZERO.
  {
    for (auto px_it = book_.begin(); px_it != book_.end(); ++px_it)
      if (px_it->second.size() > 0)
        return px_it->first;

    return 0;
  }

  template <class Q,
    typename std::enable_if< std::is_same<ask,Q>::value>::type * = nullptr>
  book_t volume_between_inclusive(book_t price_low, book_t price_high)
  // Specialization for ask quotes
  {
    book_t volume = 0;
    auto it_stop = book_.find(++price_high);
    for (auto px_it = book_.find(price_low); px_it != it_stop; ++px_it)
      for (auto elem : px_it->second)
        volume += elem.quantity;

    return volume;
  }

  template <class Q,
    typename std::enable_if< std::is_same<bid,Q>::value>::type * = nullptr>
  book_t volume_between_inclusive(book_t price_low, book_t price_high)
  // Specialization for bid quotes
  {
    book_t volume = 0;
    auto it_stop = book_.find(--price_high);
    for (auto px_it = book_.find(price_low); px_it != it_stop; ++px_it)
      for (auto elem : px_it->second)
        volume += elem.quantity;

    return volume;
  }

  std::list<T>& quotes_at_price_level(book_t price)
  {
    return book_.find(price)->second;
  }

  friend std::ostream& operator<<(std::ostream& ostream, book& bk)
  // only used for error checking
  {
    for (auto px_it = bk.book_.begin(); px_it != bk.book_.end(); ++px_it)
    {
      ostream << "price level: " << px_it->first << std::endl;
      auto px_list = px_it->second;
      for (auto elem : px_list)
        ostream << "order id: " << elem.order_id << std::endl;
    }
    return ostream;
  }


private:
  book_type book_;
  book_t num_quotes_{0};
};

#endif //TRADING_SYSTEM_COMPONENTS_BOOK_HPP

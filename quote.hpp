//
// Created by eli on 8/5/17.
//

#ifndef QUOTE_QUOTE_HPP
#define QUOTE_QUOTE_HPP

#include <cstdint>
#include "../circular_buffer/aligned_circular_buffer.hpp"
#include "../trading_system_components/ipc_messages.hpp"

//using namespace aligned;

using quote_size_t = std::uint64_t;

template <typename T>
class quote
{
private:
  T& T_this() { return *static_cast<T*>(this); }
  const T& T_this() const { return *static_cast<const T*>(this); }

public:
  quote() = default;

  explicit quote(const aligned::message_t& msg)
    : order_type(msg.type), symbol(msg.symbol), price(msg.price),
      quantity(msg.quantity), side(msg.side), venue(msg.venue),
      order_id(msg.order_id), entry_time(msg.entry_time)
  {}

  quote(const quote&) = default;
  quote& operator=(const quote&) = default;
  quote(quote&&) noexcept = default;
  quote& operator=(quote&&) noexcept = default;
  ~quote() = default;

  // operator== implemented by T
  template <typename U>
  bool operator!=(const U& o) const { return !(T_this() == o); }

  // operator< implemented by T
  template <typename U>
  bool operator>=(const U& o) const { return !(T_this() <  o); }

  // operator> implemented by T
  template <typename U>
  bool operator<=(const U& o) const { return !(T_this() >  o); }


  //quote_size_t symbol = 0; (previously)
  // quote_size_t venue = 0; (previously)
  aligned::order_type order_type = aligned::order_type::none;
  aligned::symbol_name symbol = aligned::symbol_name::none;
  aligned::venue_name venue = aligned::venue_name::none;
  aligned::side_type side = aligned::side_type::none;
  quote_size_t price = 0;
  quote_size_t quantity = 0;
  quote_size_t order_id = 0;
  quote_size_t entry_time = 0;
};


class bid : public quote<bid>
// bids should be ordered from highest to lowest, since the best bid is highest
// and we'll often be referencing the current best bid and need to do so as
// quickly as possible.
{
public:
  using quote<bid>::quote;

  bool operator<(const bid& b) const
  {
    return (price > b.price) ||
           ((price == b.price) && (entry_time < b.entry_time));
  }

  bool operator<(const quote_size_t price_value)
  {
    return (price > price_value);
  }

  bool operator>(const bid& b) const
  {
    return (price < b.price) ||
           ((price == b.price) && (entry_time > b.entry_time));
  }

  bool operator==(const bid& b) const
  {
    return (venue == b.venue) && (order_id == b.order_id);
  }

  bool operator==(const aligned::aligned_t id)
  {
    return order_id == id;
  }

};



class ask : public quote<ask>
// asks (offers) will be ordered in a straight forward manner, since the best
// offer will be that which is lowest.
{
public:
  using quote<ask>::quote;

  bool operator<(const ask& a) const
  {
    return (price < a.price) ||
           ((price == a.price) && (entry_time < a.entry_time));
  }

  bool operator<(const quote_size_t price_value)
  {
    return (price < price_value);
  }

  bool operator>(const ask& a) const
  {
    return (price > a.price) ||
           ((price == a.price) && (entry_time > a.entry_time));
  }

  bool operator==(const ask& a) const
  {
    return (venue == a.venue) && (order_id == a.order_id);
  }

  bool operator==(const aligned::aligned_t id)
  {
    return order_id == id;
  }
};



#endif //QUOTE_QUOTE_HPP

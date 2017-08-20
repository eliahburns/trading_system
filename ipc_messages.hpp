//
// Created by eli on 8/9/17.
//

#ifndef TRADING_SYSTEM_COMPONENTS_IPC_MESSAGES_HPP
#define TRADING_SYSTEM_COMPONENTS_IPC_MESSAGES_HPP


#include <cstdint>
#include <string>


/*
template<typename E>
constexpr auto to_underlying_type(E e) -> typename std::underlying_type<E>::type
// Casts an enumeration to its underlying type.
// Using to convert unsigned 64 bit integers to strings to store in MongoDB.
{
  return static_cast<typename std::underlying_type<E>::type>(e);
}
*/


namespace aligned
{

  template <typename E>
  std::string enum_str(E e)
  {
    return std::to_string(
      static_cast<typename std::underlying_type<E>::type>(e));
  }

  using aligned_t = std::uint64_t;

  enum class status_type : aligned_t
  {
    none, accepted, cancelled, rejected, filled
  };

  enum class order_type : aligned_t
  {
    none, limit, market, ioc, cancel, modify, add, delete_
  };

  enum class side_type : aligned_t
  {
    none, bid_side, ask_side
  };

  enum class venue_name : aligned_t
  {
    none, venue_one, venue_two, venue_three, venue_four
  };

  enum class symbol_name : aligned_t
  {
    none, symbol_one, symbol_two, symbol_three, symbol_four
  };

  // created by strategy, passed to order manager which will do necessary
  // checks and create internal_order_id and pass to gateway out
  struct alignas(64) to_gateway_out_t
  {
    aligned::order_type type = order_type::none;
    aligned::symbol_name symbol = symbol_name::none;
    aligned::venue_name venue = venue_name::none;
    side_type side = side_type::none;
    aligned_t price = 0;
    aligned_t quantity = 0;
    aligned_t internal_order_id = 0;
    aligned_t event_time = 0;
  };

  struct alignas(64) from_gateway_out_t
  {
    status_type status = status_type::none; // was it rejected, accepted, etc
    aligned::symbol_name symbol = symbol_name::none;
    aligned::venue_name venue = venue_name::none;
    side_type side = side_type::none;
    aligned_t price = 0;
    aligned_t quantity = 0;
    aligned_t internal_order_id = 0;
    aligned_t event_time = 0;
  };

  struct alignas(64) tob_t
  {
    aligned::symbol_name symbol = symbol_name::none;
    aligned::venue_name venue = venue_name::none;
    aligned_t bid_price = 0;
    aligned_t ask_price = 0;
    aligned_t bid_quantity = 0;
    aligned_t ask_quantity = 0;
    aligned_t book_timestamp = 0;
  };

  struct alignas(64) message_t
  {
    using msg_size_t = std::uint64_t;
    order_type type = order_type::none;
    aligned::symbol_name symbol = symbol_name::none;
    aligned::venue_name venue = venue_name::none;
    msg_size_t price = 0;
    msg_size_t quantity = 0;
    side_type side = side_type::none;
    msg_size_t order_id = 0;
    msg_size_t entry_time = 0;
  };

}

#endif //TRADING_SYSTEM_COMPONENTS_IPC_MESSAGES_HPP

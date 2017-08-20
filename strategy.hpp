//
// Created by eli on 8/9/17.
//

#ifndef TRADING_SYSTEM_COMPONENTS_STRATEGY_HPP
#define TRADING_SYSTEM_COMPONENTS_STRATEGY_HPP

#include <map>
#include <unordered_map>
#include <climits>
#include <tuple>
#include "ipc_messages.hpp"
#include "trading_system_component.hpp"
#include "quote.hpp"
#include "buffer_types.hpp"

using namespace aligned;


template <typename T, typename O>
T to_type(O o)
{ return static_cast<T>(o); };

template<class T, class F, class...Args>
T with(F f, Args&&...args)
// used to convert uint64_t to double when calculating current pnl
// while in a position
{
  return f(T(std::forward<Args>(args))...);
}

template <typename T>
class strategy : public trading_system_component<T>
// implemented methods are the default (most should be overridden)
{
public:
  using implementation = trading_system_component<T>;
  friend implementation;

  using strat_t = std::uint64_t;

  // keep track of outstanding orders in multiple markets and venues mapped
  // to symbol, which will have quotes that can belong to different exchanges.
  using bids = std::multimap<aligned::symbol_name, bid>;
  using asks = std::multimap<aligned::symbol_name, ask>;
  using order_id_map = std::unordered_map<strat_t , strat_t>;

  // symbol -> tob (for each venue)
  using symbol_tob_markets = std::multimap<aligned::symbol_name, tob_t>;

  // all symbols strategy would like to trade, mapped to all venues trader would
  // like to trade symbol at
  using symbol_markets =
  std::multimap<aligned::symbol_name, aligned::venue_name>;

  using bks_to_strat_vec =
  std::vector<std::reference_wrapper<bk_to_strat_buffer>>;

  using position_map = std::map<aligned::symbol_name, double>;
  using pnl_map = std::map<aligned::symbol_name, double>;


  strategy(strat_t id,
           om_to_strat_buffer& om_to_strat_buffer1,
           strat_to_om_buffer& strat_to_om_buffer1);

  void component_main_loop();

  void handle_update();

  void handle_response() { ; }

  const bool is_ready() { return implementation::component_ready_; }

  void log_update() { ; }

  virtual void signal() { return (*static_cast<T*>(this)).signal(); }

  virtual void execution() { return (*static_cast<T*>(this)).execution(); }

  virtual void wait_till_ready()
  { return (*static_cast<T*>(this)).wait_till_ready(); }

  void
  register_book_buffer(std::reference_wrapper<bk_to_strat_buffer> bk_to_strat);

  aligned::to_gateway_out_t
  quote_to_gateway_type(bid& b, aligned::order_type o_type);

  aligned::to_gateway_out_t
  quote_to_gateway_type(ask& a, aligned::order_type o_type);

  bid generate_bid(aligned::order_type order_type,
                    aligned::symbol_name symbol,
                    aligned::venue_name venue,
                    quote_size_t px, quote_size_t qty,
                    quote_size_t order_id, quote_size_t entry_time
  );

  ask generate_ask(aligned::order_type order_type,
                   aligned::symbol_name symbol,
                   aligned::venue_name venue,
                   quote_size_t px, quote_size_t qty,
                   quote_size_t order_id, quote_size_t entry_time
  );

  void register_market(aligned::symbol_name symbol, aligned::venue_name venue);

  aligned::aligned_t total_markets_registered(aligned::symbol_name symbol);

  const double current_position(aligned::symbol_name symbol) const;

  const double current_pnl(aligned::symbol_name symbol) const;

  const double current_unrealized_pnl(aligned::symbol_name symbol) const;

  const double theoretical_exit_price(aligned::symbol_name symbol) const;

  void update_position(aligned::symbol_name symbol, aligned::side_type side,
    aligned::aligned_t px, aligned::aligned_t qty);

  void reset_realized_pnl(aligned::symbol_name symbol);

  void update_symbol_tob_market(const aligned::tob_t& tob);

  void remove_quote(bid& b);

  void remove_quote(ask& a);

  void remove_quote(std::multimap<aligned::symbol_name, bid>::iterator b_it);

  void remove_quote(std::multimap<aligned::symbol_name, ask>::iterator a_it);

  void remove_quote(const aligned::from_gateway_out_t& from_gateway_out);

  std::multimap<aligned::symbol_name, bid>::iterator find_quote(bid& b);

  std::multimap<aligned::symbol_name, ask>::iterator find_quote(ask& a);

  std::multimap<aligned::symbol_name, ask>::iterator
  find_ask_quote(const from_gateway_out_t& from_gateway_out);

  std::multimap<aligned::symbol_name, bid>::iterator
  find_bid_quote(const from_gateway_out_t& from_gateway_out);

  std::multimap<aligned::symbol_name, ask>::iterator
  find_ask_quote(const to_gateway_out_t& to_gateway_out);

  std::multimap<aligned::symbol_name, bid>::iterator
  find_bid_quote(const to_gateway_out_t& to_gateway_out);

  void adjust_quote(
    std::multimap<aligned::symbol_name, bid>::iterator b_it,
    const aligned::from_gateway_out_t& from_gateway_out);

  void adjust_quote(
    std::multimap<aligned::symbol_name, ask>::iterator a_it,
    const aligned::from_gateway_out_t& from_gateway_out);

  // TODO : fix adjust quote to remove the remaining quantity if type is IOC
  void adjust_quote(const aligned::from_gateway_out_t& from_gateway_out);

  void add_quote(const bid& b); //{ bids_.emplace(b.symbol, b); }

  void add_quote(const ask& a);  //{ asks_.emplace(a.symbol, a); }

  aligned::aligned_t total_quantity_quoting();

  const bool quote_already_exists(to_gateway_out_t to_gateway_out);

  const bids& get_all_bids() { return bids_; }

  const asks& get_all_asks() { return asks_; }

  const aligned::aligned_t generate_new_order_id() { return ++strat_order_id_; }

  // TODO: implement database tracking of orders for viewing by trader

  void add_quote_database(const bid& b);
  void add_quote_database(const ask& a);

  void update_quote_database(const bid& b);
  void update_quote_database(const ask& a);

  void remove_quote_database(const bid& b);
  void remove_quote_database(const ask& a);

  void drop_all_orders();

  const bool has_position();

  void log_pnl();

protected:
  aligned::om_to_strat_buffer& om_to_strat_buffer_;
  aligned::strat_to_om_buffer& strat_to_om_buffer_;

  position_map net_position_map_; // for symbol over all exchanges
  pnl_map realized_pnl_map_; // for symbol over all exchanges
  pnl_map unrealized_pnl_map_; // for symbol over all exchanges

  strat_t strat_order_id_;

  bids bids_;
  asks asks_;
  order_id_map order_id_map_; // key: strat order ids, value: order manger ids

  symbol_tob_markets symbol_tob_markets_; // symbol -> tob (for each venue)
  symbol_markets symbol_markets_;
  bks_to_strat_vec bks_to_strat_vec_;
  std::string collection_name_;
};


template <typename T>
strategy<T>::strategy(strategy::strat_t id,
                      om_to_strat_buffer &om_to_strat_buffer1,
                      strat_to_om_buffer &strat_to_om_buffer1)
  : trading_system_component<T>(id),
    om_to_strat_buffer_(om_to_strat_buffer1),
    strat_to_om_buffer_(strat_to_om_buffer1),
    strat_order_id_{0}, collection_name_{""}
{
  collection_name_ = "strategy" + std::to_string(implementation::component_id_);
  implementation::component_ready_ = true;
}


template <typename T>
inline aligned::to_gateway_out_t
strategy<T>::quote_to_gateway_type(ask &a, aligned::order_type o_type)
{
  aligned::to_gateway_out_t gw_out{};
  gw_out.type =  o_type;
  gw_out.symbol = a.symbol;
  gw_out.price = a.price;
  gw_out.quantity = a.quantity;
  gw_out.side = aligned::side_type::ask_side;
  gw_out.venue = a.venue;
  gw_out.internal_order_id = a.order_id;
  gw_out.event_time = implementation::get_time();
  return gw_out;
}


template <typename T>
inline aligned::to_gateway_out_t
strategy<T>::quote_to_gateway_type(bid &b, aligned::order_type o_type)
{
  aligned::to_gateway_out_t gw_out{};
  gw_out.type =  o_type;
  gw_out.symbol = b.symbol;
  gw_out.price = b.price;
  gw_out.quantity = b.quantity;
  gw_out.side = aligned::side_type::bid_side;
  gw_out.venue = b.venue;
  gw_out.internal_order_id = b.order_id;
  gw_out.event_time = implementation::get_time();
  return gw_out;
}


template <typename T>
inline void strategy<T>::handle_update()
{
  for (bk_to_strat_buffer& elem : bks_to_strat_vec_)
    if (!elem.empty())
    {
      tob_t tob = elem.pop_front();
      std::cout << "bid/ask : " << tob.bid_price << "/" << tob.ask_price
                << " "
                  "(" << tob.bid_quantity << "/" << tob.ask_quantity << ")"
                << std::endl;
    }
}


template <typename T>
void strategy<T>::component_main_loop()
{
  while (implementation::component_ready_)
  {
    handle_update();
  }
}


template <typename T>
inline void strategy<T>::register_book_buffer(
  std::reference_wrapper<bk_to_strat_buffer> bk_to_strat)
{
  bks_to_strat_vec_.push_back(bk_to_strat);
}

template <typename T>
inline bid
strategy<T>::generate_bid(aligned::order_type order_type,
                          aligned::symbol_name symbol,
                          aligned::venue_name venue,
                          quote_size_t px, quote_size_t qty,
                          quote_size_t order_id, quote_size_t entry_time)
// quote data attributes:
// aligned::order_type order_type = aligned::order_type::none;
// aligned::symbol_name symbol = aligned::symbol_name::none;
// aligned::venue_name venue = aligned::venue_name::none;
// aligned::side_type side = aligned::side_type::none;
// quote_size_t price = 0;
// quote_size_t quantity = 0;
// quote_size_t order_id = 0;
// quote_size_t entry_time = 0;
{
  bid b{};
  b.order_type = order_type;
  b.symbol = symbol;
  b.venue = venue;
  b.side = aligned::side_type::bid_side;
  b.price = px;
  b.quantity = qty;
  b.order_id = order_id;
  b.entry_time = entry_time;
  return b;
}

template <typename T>
inline ask
strategy<T>::generate_ask(aligned::order_type order_type,
                       aligned::symbol_name symbol,
                       aligned::venue_name venue, quote_size_t px,
                       quote_size_t qty, quote_size_t order_id,
                       quote_size_t entry_time)
// quote data attributes:
// aligned::order_type order_type = aligned::order_type::none;
// aligned::symbol_name symbol = aligned::symbol_name::none;
// aligned::venue_name venue = aligned::venue_name::none;
// aligned::side_type side = aligned::side_type::none;
// quote_size_t price = 0;
// quote_size_t quantity = 0;
// quote_size_t order_id = 0;
// quote_size_t entry_time = 0;
{
  ask a{};
  a.order_type = order_type;
  a.symbol = symbol;
  a.venue = venue;
  a.side = aligned::side_type::ask_side;
  a.price = px;
  a.quantity = qty;
  a.order_id = order_id;
  a.entry_time = entry_time;
  return a;
}

template <typename T>
void strategy<T>::register_market(aligned::symbol_name symbol,
                               aligned::venue_name venue)
// Place new symbol into position map and pnl map. Put the new venue
// corresponding to the symbol into the symbol markets multimap.
{
  net_position_map_.emplace(symbol, 0);
  realized_pnl_map_.emplace(symbol, 0);
  unrealized_pnl_map_.emplace(symbol, 0);
  symbol_markets_.emplace(symbol, venue);
}

template <typename T>
inline const double
strategy<T>::current_position(aligned::symbol_name symbol) const
{
  return net_position_map_.find(symbol)->second;
}

template <typename T>
inline const double
strategy<T>::current_pnl(aligned::symbol_name symbol) const
{
  double cur_pnl = 0;
  auto cur_realized_pnl = realized_pnl_map_.find(symbol)->second;
  auto cur_pos = current_position(symbol);
  if (cur_pos != 0)
  {
    cur_pnl = unrealized_pnl_map_.find(symbol)->second;
    cur_pnl += cur_pos * theoretical_exit_price(symbol);
  }
  return cur_pnl + cur_realized_pnl;
}

template <typename T>
inline const double
strategy<T>::theoretical_exit_price(aligned::symbol_name symbol) const
// Simple, could be improved, theoretical exit price that is just the best
// weighted average top of book prices at any venue for the symbol.
{
  auto theo = [](double&& b_px, double&& a_px, double&& b_qty,
                  double&& a_qty)
  { return (b_px*b_qty + a_px*a_qty) / (b_qty+a_qty); };

  double theo_exit = 0;
  auto cur_pos = current_position(symbol);
  double best_exit = (cur_pos > 0) ? 0 : std::numeric_limits<double>::max();
  auto ret = symbol_tob_markets_.equal_range(symbol);

  for (auto it = ret.first; it != ret.second; ++it)
  {
    const tob_t tob = it->second;
    theo_exit = with<double>(theo, tob.bid_price, tob.ask_price,
                             tob.bid_quantity, tob.ask_quantity);
    if (cur_pos > 0 && theo_exit > best_exit)
      best_exit = theo_exit;
    else if (cur_pos < 0 && theo_exit < best_exit)
      best_exit = theo_exit;
  }
  return theo_exit;
}

template <typename T>
inline void
strategy<T>::update_position(aligned::symbol_name symbol,
                             aligned::side_type side, aligned::aligned_t px,
                             aligned::aligned_t qty)
// Adjust the net position and unrealized pnl. If the net position is flat
// afterwards, move whatever is left in unrealized pnl to realized pnl and
// set unrealized pnl to zero.
{
  const double scalar = (side == aligned::side_type::bid_side) ? 1 : -1;
  double net_pos = net_position_map_.find(symbol)->second; // must update l8r

  auto pos_adj =  qty * scalar;
  unrealized_pnl_map_.find(symbol)->second -= px * pos_adj;
  net_position_map_.find(symbol)->second += pos_adj;
  //double unreal = unrealized_pnl_map_.find(symbol)->second;
  if ((net_pos + pos_adj) == 0)
  {
    realized_pnl_map_.find(symbol)->second +=
      unrealized_pnl_map_.find(symbol)->second;

    unrealized_pnl_map_.find(symbol)->second = 0;
  }
}

template <typename T>
inline void
strategy<T>::reset_realized_pnl(aligned::symbol_name symbol)
/// Reset current realized PnL corresponding to a symbol and its markets.
/// \tparam T type of strategy.
/// \param symbol name of symbol for which to reset pnl.
{
  realized_pnl_map_.find(symbol)->second = 0;
}

template <typename T>
inline void
strategy<T>::update_symbol_tob_market(const aligned::tob_t& tob)
/// Updates the tracked top of book values corresponding to symbol and venue.
/// \tparam T type of strategy.
/// \param tob top of book object, defined in ipc_messages.cpp
{
  auto eq_range = symbol_tob_markets_.equal_range(tob.symbol);
  bool found_tob = false;
  for (auto it = eq_range.first; it != eq_range.second; ++it)
  {
    tob_t& ref_tob = it->second;
    if (ref_tob.venue == tob.venue)
    {
      ref_tob = tob;
      found_tob = true;
    }
  }
  if (!found_tob)
    symbol_tob_markets_.emplace(tob.symbol, tob);
}

template <typename T>
aligned::aligned_t
strategy<T>::total_markets_registered(aligned::symbol_name symbol)
/// count the number of venues (markets) where the symbol has been registered.
/// \tparam T type of strategy
/// \param symbol name of symbol to look up number of venues strategy knows of.
/// \return number of venues corresponding to symbol.
{
  auto ret = symbol_markets_.equal_range(symbol);
  aligned::aligned_t mkt_count = 0;

  for (auto it = ret.first; it != ret.second; ++it)
    ++mkt_count;

  return mkt_count;
}

template <typename T>
inline void strategy<T>::remove_quote(bid &b)
{
  auto ret = bids_.equal_range(b.symbol);
  for (auto it = ret.first; it != ret.second; ++it)
    if (it->second.order_id == b.order_id)
    {
      bids_.erase(it);
      return;
    }
}

template <typename T>
inline void strategy<T>::remove_quote(ask& a)
{
  auto ret = asks_.equal_range(a.symbol);
  for (auto it = ret.first; it != ret.second; ++it)
    if (it->second.order_id == a.order_id)
    {
      asks_.erase(it);
      return;
    }
}

template <typename T>
inline std::multimap<aligned::symbol_name, bid>::iterator
 strategy<T>::find_quote(bid &b)
{
  auto ret = bids_.equal_range(b.symbol);
  for (auto it = ret.first; it != ret.second; ++it)
    if (it->second.order_id == b.order_id)
      return it;
}

template <typename T>
inline std::multimap<aligned::symbol_name, ask>::iterator
strategy<T>::find_quote(ask& a)
{
  auto ret = asks_.equal_range(a.symbol);
  for (auto it = ret.first; it != ret.second; ++it)
    if (it->second.order_id == a.order_id)
      return it;
}

template <typename T>
inline void strategy<T>::remove_quote(
  std::multimap<aligned::symbol_name, bid>::iterator b_it)
{
  bids_.erase(b_it);
}

template <typename T>
inline void strategy<T>::remove_quote(
  std::multimap<aligned::symbol_name, ask>::iterator a_it)
{
  asks_.erase(a_it);
}

template <typename T>
inline std::multimap<aligned::symbol_name, ask>::iterator
strategy<T>::find_ask_quote(const from_gateway_out_t& from_gateway_out)
{
  auto ret = asks_.equal_range(from_gateway_out.symbol);
  auto it = ret.first;
  for ( ; it != ret.second; ++it)
    if (it->second.order_id == from_gateway_out.internal_order_id)
      return it;
  if (ret.first == ret.second)
    if (ret.first->second.order_id == from_gateway_out.internal_order_id)
      return it;
}

template <typename T>
inline std::multimap<aligned::symbol_name, bid>::iterator
strategy<T>::find_bid_quote(const from_gateway_out_t& from_gateway_out)
{
  auto ret = bids_.equal_range(from_gateway_out.symbol);
  auto it = ret.first;
  for ( ; it != ret.second; ++it)
    if (it->second.order_id == from_gateway_out.internal_order_id)
      return it;
  if (ret.first == ret.second)
    if (ret.first->second.order_id == from_gateway_out.internal_order_id)
      return it;
}

template <typename T>
inline void
strategy<T>::remove_quote(const aligned::from_gateway_out_t& from_gateway_out)
{
  if (from_gateway_out.side == aligned::side_type::ask_side)
  {
    auto it = find_ask_quote(from_gateway_out);
    remove_quote(it);
  }
  else //  side of order to remove is bid
  {
    auto it = find_bid_quote(from_gateway_out);
    remove_quote(it);
  }
}

template <typename T>
inline void strategy<T>::adjust_quote(
  std::multimap<aligned::symbol_name, bid>::iterator b_it,
  const aligned::from_gateway_out_t& from_gateway_out)
{
  bid& b = b_it->second;
  b.quantity -= from_gateway_out.quantity;
  update_quote_database(b);
  if (b.quantity == 0 || b.order_type == order_type::ioc)
    remove_quote(b_it);
}

template <typename T>
inline void strategy<T>::adjust_quote(
  std::multimap<aligned::symbol_name, ask>::iterator a_it,
  const aligned::from_gateway_out_t& from_gateway_out)
{
  ask& a = a_it->second;
  a.quantity -= from_gateway_out.quantity;
  update_quote_database(a);
  if (a.quantity == 0 || a.order_type == order_type::ioc)
    remove_quote(a_it);
}

template <typename T>
inline void
strategy<T>::adjust_quote(const aligned::from_gateway_out_t& from_gateway_out)
{
  if (from_gateway_out.side == aligned::side_type::ask_side)
  {
    auto it = find_ask_quote(from_gateway_out);
    adjust_quote(it, from_gateway_out);
  }
  else //  side of order partial fill is bid side
  {
    auto it = find_bid_quote(from_gateway_out);
    adjust_quote(it, from_gateway_out);
  }
}

template <typename T>
inline aligned::aligned_t strategy<T>::total_quantity_quoting()
{
  aligned::aligned_t total_q = 0;
  for (auto it = bids_.begin(); it != bids_.end(); ++it)
    total_q += it->second.quantity;
  for (auto it = asks_.begin(); it != asks_.end(); ++it)
    total_q += it->second.quantity;
  return total_q;
}

template <typename T>
inline const bool strategy<T>::quote_already_exists(
  to_gateway_out_t to_gateway_out)
{
  if (to_gateway_out.side == side_type::ask_side)
  {
    auto it = find_ask_quote(to_gateway_out);
    ask& a = it->second;
    if (to_gateway_out.price == a.price || to_gateway_out.type ==
                                           order_type::market)
      if (to_gateway_out.venue == a.venue)
        if (to_gateway_out.quantity == a.quantity)
            if (to_gateway_out.type == a.order_type)
              return true;
  }
  else
  {
    auto it = find_bid_quote(to_gateway_out);
    bid& b = it->second;
    if (to_gateway_out.price == b.price || to_gateway_out.type ==
                                             order_type::market)
      if (to_gateway_out.quantity == b.quantity)
        if (to_gateway_out.venue == b.venue)
            if (to_gateway_out.type == b.order_type)
              return true;
  }
  return false;
}

template <typename T>
std::multimap<aligned::symbol_name, ask>::iterator
strategy<T>::find_ask_quote(const to_gateway_out_t &to_gateway_out)
{
  auto ret = asks_.equal_range(to_gateway_out.symbol);
  auto it = ret.first;
  for ( ; it != ret.second; ++it)
    if (it->second.order_id == to_gateway_out.internal_order_id)
      return it;
  if (ret.first == ret.second && ret.first != asks_.end())
    if (ret.first->second.order_id == to_gateway_out.internal_order_id)
      return it;
  return asks_.end();
}

template <typename T>
std::multimap<aligned::symbol_name, bid>::iterator
strategy<T>::find_bid_quote(const to_gateway_out_t &to_gateway_out)
{
  auto ret = bids_.equal_range(to_gateway_out.symbol);
  auto it = ret.first;
  for ( ; it != ret.second; ++it)
    if (it->second.order_id == to_gateway_out.internal_order_id)
      return it;
  if (ret.first == ret.second && ret.first != bids_.end())
    if (ret.first->second.order_id == to_gateway_out.internal_order_id)
      return it;
  return bids_.end();
}

template <typename T>
inline const double
strategy<T>::current_unrealized_pnl(aligned::symbol_name symbol) const
{
  double cur_pnl = 0;
  auto cur_pos = current_position(symbol);
  if (cur_pos != 0)
  {
    cur_pnl = unrealized_pnl_map_.find(symbol)->second;
    cur_pnl += cur_pos * theoretical_exit_price(symbol);
  }
  return cur_pnl;
}

template <typename T>
void strategy<T>::add_quote_database(const bid& b)
{
  std::string orders{"outstanding_orders"};
  auto collection = implementation::conn_[collection_name_][orders];
  auto builder = bsoncxx::builder::stream::document{};

  bsoncxx::document::value doc_value = builder
    << "order_id" << std::to_string(b.order_id)
    << "event_time" << std::to_string(b.entry_time)
    << "status" << "pending"
    << "symbol" << enum_str(b.symbol)
    << "venue" << enum_str(b.venue)
    << "price" << std::to_string(b.price)
    << "quantity" << std::to_string(b.quantity)
    << "side" << enum_str(b.side)
    << "type" << enum_str(b.order_type)
    << bsoncxx::builder::stream::finalize;

  collection.insert_one(doc_value.view());
}

template <typename T>
void strategy<T>::add_quote_database(const ask& a)
{
  std::string orders{"outstanding_orders"};
  auto collection = implementation::conn_[collection_name_][orders];
  auto builder = bsoncxx::builder::stream::document{};

  bsoncxx::document::value doc_value = builder
    << "order_id" << std::to_string(a.order_id)
    << "event_time" << std::to_string(a.entry_time)
    << "status" << "pending"
    << "symbol" << enum_str(a.symbol)
    << "venue" << enum_str(a.venue)
    << "price" << std::to_string(a.price)
    << "quantity" << std::to_string(a.quantity)
    << "side" << enum_str(a.side)
    << "type" << enum_str(a.order_type)
    << bsoncxx::builder::stream::finalize;
  collection.insert_one(doc_value.view());
}

template <typename T>
void strategy<T>::update_quote_database(const bid& b)
{
  // key by order id
  std::string orders{"outstanding_orders"};
  auto collection = implementation::conn_[collection_name_][orders];
  auto id = std::to_string(b.order_id);

  if (b.quantity != 0)
  {
    auto qty = std::to_string(b.quantity);
    collection.update_one(
      document{} << "order_id" << id << finalize,
      document{} << "$set" << open_document << "quantity"
                 << qty <<  "status" <<
                 "partially filled" << close_document << finalize);
  }
  else if (b.quantity == 0 || b.order_type == order_type::ioc)
  {
    collection.delete_one(document{} << "order_id" << id << finalize);
  }

}

template <typename T>
void strategy<T>::update_quote_database(const ask& a)
{
  // key by order id
  std::string orders{"outstanding_orders"};
  auto collection = implementation::conn_[collection_name_][orders];
  auto id = std::to_string(a.order_id);

  if (a.quantity != 0)
  {
    auto qty = std::to_string(a.quantity);
    collection.update_one(
      document{} << "order_id" << id << finalize,
      document{} << "$set" << open_document << "quantity"
                 << qty <<  "status" <<
                 "partially filled" << close_document << finalize);
  }
  else if (a.quantity == 0 || a.order_type == order_type::ioc)
  {
    collection.delete_one(document{} << "order_id" << id << finalize);
  }

}

template <typename T>
void strategy<T>::drop_all_orders()
{
  std::string orders{"outstanding_orders"};
  auto collection = implementation::conn_[collection_name_][orders];
  collection.drop();
}

template <typename T>
void strategy<T>::add_quote(const bid &b)
{
  bids_.emplace(b.symbol, b);
  add_quote_database(b);
  log_pnl();

}

template <typename T>
void strategy<T>::add_quote(const ask& a)
{
  asks_.emplace(a.symbol, a);
  add_quote_database(a);
  log_pnl();
}

template <typename T>
const bool strategy<T>::has_position()
{
  for (auto it = symbol_markets_.begin(); it != symbol_markets_.end(); ++it)
    if (current_position(it->first))
      return true;
  return false;
}

template <typename T>
void strategy<T>::log_pnl()
{
    using namespace aligned;
    std::string pnl{"PnL"};
    auto collection = implementation::conn_[collection_name_][pnl];

    const double p = 0;
    auto symbol = symbol_name::none;
    for (auto elem : symbol_markets_)
        if (elem.first != symbol)
        {
          symbol = elem.first;
          const double p = current_pnl(symbol);
          auto builder = bsoncxx::builder::stream::document{};
          bsoncxx::document::value doc_value = builder
            << "symbol" << symbol_to_str(symbol)
            << "event_time" << std::to_string(implementation::get_time())
            << "pnl" << std::to_string(p)
            << bsoncxx::builder::stream::finalize;
          collection.insert_one(doc_value.view());
        }

}

#endif //TRADING_SYSTEM_COMPONENTS_STRATEGY_HPP

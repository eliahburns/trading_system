//
// Created by eli on 8/14/17.
//

#include "arbitrage_trader.hpp"

void arbitrage_trader::signal()
// Check if signal buffer has a new tob update, then go through all tob's for
// the same symbol at other venues. If bid at venue X is greater than ask at
// venue Y, send ask to venue X and bid to venue Y as IOC and record orders
// sent in strategy bids, asks (handle_response will deal with partial fills)
{
  if (!signal_buffer_.empty())
  {
    aligned::tob_t tob{};
    tob = signal_buffer_front();
    update_symbol_tob_market(tob);
    auto ret = symbol_tob_markets_.equal_range(tob.symbol);
    for (auto tob_it = ret.first; tob_it != ret.second; ++tob_it)
    {
      aligned::tob_t& tob_it_ref = tob_it->second;
      if ((tob_it_ref.venue != tob.venue) && (tob_it_ref.symbol == tob.symbol)
        && open_new_trades_)
      {
        if (tob_it_ref.bid_price > tob.ask_price)
        {
          auto max_qty = std::min(tob_it_ref.bid_quantity, tob.ask_quantity);

          aligned::to_gateway_out_t ask_to_gw{};
          ask_to_gw.type = order_type::ioc;
          ask_to_gw.internal_order_id = ++strat_order_id_;
          ask_to_gw.venue = tob_it_ref.venue;
          ask_to_gw.symbol = tob_it_ref.symbol;
          ask_to_gw.price = tob_it_ref.bid_price;
          ask_to_gw.quantity = max_qty;
          ask_to_gw.side = side_type::ask_side;
          ask_to_gw.event_time = implementation::get_time();

          aligned::to_gateway_out_t bid_to_gw{};
          bid_to_gw.type = order_type::ioc;
          bid_to_gw.internal_order_id = ++strat_order_id_;
          bid_to_gw.venue = tob.venue;
          bid_to_gw.symbol = tob.symbol;
          bid_to_gw.price = tob.ask_price;
          bid_to_gw.quantity = max_qty;
          bid_to_gw.side = side_type::bid_side;
          bid_to_gw.event_time = implementation::get_time();

          to_execution_buffer(bid_to_gw);
          to_execution_buffer(ask_to_gw);
        }
        else if (tob.bid_price > tob_it_ref.ask_price)
        {
          // TODO: need to check if trade has already been placed
          auto max_qty = std::min(tob_it_ref.ask_quantity, tob.bid_quantity);

          aligned::to_gateway_out_t bid_to_gw{};
          bid_to_gw.type = order_type::ioc;
          bid_to_gw.internal_order_id = generate_new_order_id();
          bid_to_gw.venue = tob_it_ref.venue;
          bid_to_gw.symbol = tob_it_ref.symbol;
          bid_to_gw.price = tob_it_ref.ask_price;
          bid_to_gw.quantity = max_qty;
          bid_to_gw.side = side_type::bid_side;
          bid_to_gw.event_time = implementation::get_time();

          aligned::to_gateway_out_t ask_to_gw{};
          ask_to_gw.type = order_type::ioc;
          ask_to_gw.internal_order_id = generate_new_order_id();
          ask_to_gw.venue = tob.venue;
          ask_to_gw.symbol = tob.symbol;
          ask_to_gw.price = tob.bid_price;
          ask_to_gw.quantity = max_qty;
          ask_to_gw.side = side_type::ask_side;
          ask_to_gw.event_time = implementation::get_time();

          to_execution_buffer(bid_to_gw);
          to_execution_buffer(ask_to_gw);
        }
      }
    }
    check_stop_loss(tob.symbol); // get out of position if past stop loss
  }
}

void arbitrage_trader::execution()
{
  while (!execution_buffer_.empty())
  {
    auto to_gateway_out = execution_buffer_front();
    // check if trade has already been placed. if so, then don't send duplicate
    // trade to order manager
    // TODO: move exit bad position logic to here
    if (!quote_just_sent(to_gateway_out))
    {
      strat_to_om_buffer_.push_back(to_gateway_out);
      add_quote_sent(to_gateway_out);
      auto& tgo = to_gateway_out;
      if (to_gateway_out.side == side_type::ask_side)
      {
        ask a = generate_ask(tgo.type, tgo.symbol, tgo.venue, tgo.price,
                             tgo.quantity, tgo.internal_order_id,
                             implementation::get_time());
        add_quote(a);
      }
      else
      {
        bid b = generate_bid(tgo.type, tgo.symbol, tgo.venue, tgo.price,
                             tgo.quantity, tgo.internal_order_id,
                             implementation::get_time());
        add_quote(b);
      }
    }
  }
}

void arbitrage_trader::handle_update()
// check for update, pass tob update to signal, and log tob update
{
  for (bk_to_strat_buffer& elem : bks_to_strat_vec_)
    if (!elem.empty())
    {
      tob_t tob = elem.pop_front();
      to_signal_buffer(tob);
    }
}

void arbitrage_trader::handle_response()
// check if order is filled accepted or rejected
{
  if (!om_to_strat_buffer_.empty())
  {
    aligned::from_gateway_out_t from_gateway_out =
      om_to_strat_buffer_.pop_front();
    cleanup_quote_sent(from_gateway_out);
    // do nothing if we see that order was merely accepted
    if (from_gateway_out.status == status_type::rejected)
      // find order in bids/asks and remove from internal data struct
      remove_quote(from_gateway_out);
    else if (from_gateway_out.status == status_type::filled)
    {
      update_position(from_gateway_out.symbol, from_gateway_out.side,
                      from_gateway_out.price, from_gateway_out.quantity);
      // adjust quote to reflect position after fill
      adjust_quote(from_gateway_out);
    }
  }
}

void arbitrage_trader::component_main_loop()
{
  wait_till_ready();

  while (implementation::component_ready_)
  {
    handle_update();

    signal();

    execution();

    handle_response();
  }
}

void arbitrage_trader::wait_till_ready()
{
  while (!all_markets_available())
    for (bk_to_strat_buffer& elem : bks_to_strat_vec_)
      if (!elem.empty())
      {
        tob_t tob = elem.pop_front();
        // add/update tob to symbol_tob_markets
        update_symbol_tob_market(tob);
      }
}

bool arbitrage_trader::all_markets_available()
{
  for (auto elem : symbol_markets_)
  {
    aligned::symbol_name symbol = elem.first;
    aligned::venue_name venue = elem.second;
    auto mm_eq_range = symbol_tob_markets_.equal_range(symbol);
    bool found_venue = false;
    for(auto it = mm_eq_range.first; it != mm_eq_range.second; ++it)
      if (it->second.venue == venue)
        found_venue = true;

    if (!found_venue)
      return false;
  }
  return true;
}

void arbitrage_trader::add_quote_sent(const to_gateway_out_t& to_gateway_out)
/// use after sending a quote to exchange, through order manager.
/// \param to_gateway_out message to exchange.
{
  quotes_sent_.emplace(to_gateway_out.symbol, to_gateway_out);
}

void arbitrage_trader::cleanup_quote_sent(from_gateway_out_t from_gateway_out)
/// use after receiving a response from order manager.
/// \param from_gateway_out message that came from exchange, through order man.
{
  auto ret = quotes_sent_.equal_range(from_gateway_out.symbol);
  for (auto it = ret.first; it != ret.second; ++it)
    if (it->second.internal_order_id == from_gateway_out.internal_order_id)
    {
      quotes_sent_.erase(it);
      return;
    }
}

const bool arbitrage_trader::quote_just_sent(to_gateway_out_t to_gateway_out)
/// true if order with same price/quantity already sent with pending response.
/// \param to_gateway_out message to exchange, through order manager.
/// \return boolean true if order was sent and still waiting for response.
{
  auto ret = quotes_sent_.equal_range(to_gateway_out.symbol);
  for (auto it = ret.first; it != ret.second; ++it)
  {
    auto& to_gw = it->second;
    if (to_gw.venue == to_gateway_out.venue)
      if (to_gw.side == to_gateway_out.side) // mkt order may not have crt px
        if (to_gw.price == to_gateway_out.price || to_gateway_out.type ==
                                                     order_type::market)
          if (to_gw.quantity == to_gateway_out.quantity)
            return true;
  }
  return false;
}

void arbitrage_trader::check_stop_loss(symbol_name symbol)
/// check if the current position value is less than stop loss and exit position
/// \param symbol name of symbol for which to check
{
  if (current_unrealized_pnl(symbol) <= - stop_loss_)
  {
    // since all initial orders are ioc, no need to modify other orders
    double pos = current_position(symbol);
    to_gateway_out_t to_gateway_out{};
    to_gateway_out.symbol = symbol;
    to_gateway_out.type = order_type::market; // not ideal
    // we need to know the best market to go to
    tob_t best_tob{};
    // TODO : change adding order here to just forwarding message to exc buffer
    if (pos < 0) // we should buy, so we're concerned with the best ask
    {
      best_tob = best_market_to_exit_in(symbol, side_type::bid_side);
      to_gateway_out.quantity = static_cast<aligned_t>(- pos);
      to_gateway_out.side = side_type::bid_side;
      to_gateway_out.price = best_tob.ask_price; // not really used
      to_gateway_out.venue = best_tob.venue;
    }
    else
    {
      to_gateway_out.quantity = static_cast<aligned_t>(pos);
      best_tob = best_market_to_exit_in(symbol, side_type::ask_side);
      to_gateway_out.side = side_type::ask_side;
      to_gateway_out.price = best_tob.ask_price; // not really used
      to_gateway_out.venue = best_tob.venue;
    }
    to_gateway_out.internal_order_id = generate_new_order_id();
    to_gateway_out.event_time = implementation::get_time();
    to_execution_buffer(to_gateway_out);
  }
}

aligned::tob_t arbitrage_trader::best_market_to_exit_in(symbol_name symbol,
  side_type side)
{
  auto ret = symbol_tob_markets_.equal_range(symbol);
  auto it = ret.first;
  tob_t best_tob = it->second;
  if (side == side_type::ask_side)
  {
    for ( ; it != ret.second; ++it)
      if (best_tob.bid_price > it->second.bid_price)
        best_tob = it->second;
  }
  else
  {
    for ( ; it != ret.second; ++it)
      if (best_tob.ask_price < it->second.ask_price)
        best_tob = it->second;
  }
  return best_tob;
}




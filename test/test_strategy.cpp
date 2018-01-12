//
// Created by eli on 8/17/17.
//

#include "arbitrage_trader.hpp"
#include "catch.hpp"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "ClangTidyInspection"
TEST_CASE("test add/adjust/remove quote", "[strategy_quote]")
{
  using namespace aligned;
  // will register three different books
  static bk_to_strat_buffer bk_to_strat_buffer1;
  static bk_to_strat_buffer bk_to_strat_buffer2;

  static om_to_strat_buffer om_to_strat_buffer_;
  static strat_to_om_buffer strat_to_om_buffer_;

  aligned_t arb_trader_id = 5;
  arbitrage_trader arb_trader(arb_trader_id,
                              om_to_strat_buffer_,
                              strat_to_om_buffer_);

  auto symbol = symbol_name::symbol_one;
  auto venue = venue_name::venue_two;
  aligned_t id = 0;
  aligned_t b_px = 5;
  aligned_t b_qty = 11;
  aligned_t a_px = 5;
  aligned_t a_qty = 11;
  bid b = arb_trader.generate_bid(order_type::limit, symbol, venue, b_px, b_qty,
                                  ++id, arb_trader.get_time());
  ask a = arb_trader.generate_ask(order_type::limit, symbol, venue, a_px, a_qty,
                                  ++id, arb_trader.get_time());
  arb_trader.add_quote(b);
  arb_trader.add_quote(a);

  REQUIRE(arb_trader.total_quantity_quoting() == b_qty + a_qty);
  // double amount quoting at b_px and check again
  bid b2 = arb_trader.generate_bid(order_type::limit, symbol, venue, b_px, b_qty,
                                  ++id, arb_trader.get_time());
  arb_trader.add_quote(b2);
  REQUIRE(arb_trader.total_quantity_quoting() == 2*b_qty + a_qty);

  aligned_t fill_qty = 3;
  from_gateway_out_t from_gateway_out{};
  from_gateway_out.internal_order_id = 1;
  from_gateway_out.price = b_px;
  from_gateway_out.side = side_type::bid_side;
  from_gateway_out.symbol = symbol;
  from_gateway_out.venue = venue;
  from_gateway_out.status = status_type::filled;
  from_gateway_out.quantity = fill_qty;

  arb_trader.adjust_quote(from_gateway_out);
  REQUIRE(arb_trader.total_quantity_quoting() == 2*b_qty + a_qty - fill_qty);

  from_gateway_out.quantity = b_qty - fill_qty;
  arb_trader.adjust_quote(from_gateway_out);
  REQUIRE(arb_trader.total_quantity_quoting() == b_qty + a_qty);

  from_gateway_out.internal_order_id = 2;
  from_gateway_out.side = side_type::ask_side;
  from_gateway_out.quantity = a_qty;
  arb_trader.adjust_quote(from_gateway_out);
  REQUIRE(arb_trader.total_quantity_quoting() == b_qty);

  from_gateway_out.internal_order_id = id;
  from_gateway_out.side = side_type::bid_side;
  from_gateway_out.quantity = b_qty;
  arb_trader.adjust_quote(from_gateway_out);
  REQUIRE(arb_trader.total_quantity_quoting() == 0);

  aligned_t quote_limit = 25;
  aligned_t quote_sum = 0;
  for (aligned_t i = 0; i < quote_limit; ++i)
  {
    bid b = arb_trader.generate_bid(order_type::limit, symbol, venue, b_px+i,
                                    b_qty * (i+1), i, arb_trader.get_time());
    arb_trader.add_quote(b);
    quote_sum += b_qty * (i+1);
    REQUIRE(arb_trader.total_quantity_quoting() == quote_sum);
  }
  for (aligned_t i = 0; i < quote_limit; ++i)
  {
    from_gateway_out.internal_order_id = i;
    from_gateway_out.quantity = b_qty * (i+1);
    from_gateway_out.price = b_px + i;
    arb_trader.adjust_quote(from_gateway_out);
    quote_sum -= b_qty * (i+1);
    REQUIRE(arb_trader.total_quantity_quoting() == quote_sum);
  }
}

TEST_CASE("test a cycle of trades through arbitrage_trader and confirm pnl is"
            "correct", "[test_cycle_of_trades]")
{

  using namespace aligned;
  // will register three different books
  static bk_to_strat_buffer bk_to_strat_buffer1;
  static bk_to_strat_buffer bk_to_strat_buffer2;

  static om_to_strat_buffer om_to_strat_buffer_;
  static strat_to_om_buffer strat_to_om_buffer_;

  aligned_t arb_trader_id = 5;
  arbitrage_trader arb_trader(arb_trader_id,
                              om_to_strat_buffer_,
                              strat_to_om_buffer_);

  arb_trader.register_book_buffer(bk_to_strat_buffer1);
  arb_trader.register_book_buffer(bk_to_strat_buffer2);

  // using 2 different venues and one symbol
  auto symbol = symbol_name::symbol_four; // arbitrarily chosen
  auto venue1 = venue_name::venue_one; // arbitrarily chosen
  auto venue2 = venue_name::venue_two; // arbitrarily chosen

  arb_trader.register_market(symbol, venue1);
  arb_trader.register_market(symbol, venue2);

  aligned_t limit = 10;
  const aligned_t quantity = 5;
  const aligned_t cleanup_cycle = 0;
  unsigned trade_opportunity_count = 0;
  unsigned profit_per_trade = 1;
  for (aligned_t j = 0; j < limit + cleanup_cycle; ++j)
  {
    // put new tob into each buffer
    tob_t tob{};
    tob.venue = venue2;
    tob.bid_price = 6;
    tob.ask_price = 7;
    tob.bid_quantity = quantity;
    tob.ask_quantity = quantity;
    tob.symbol = symbol;
    bk_to_strat_buffer2.push_back(tob);
    tob.venue = venue1;
    if (j > 1 && j < 10 && j % 2 == 0)
    {
      ++trade_opportunity_count;
      tob.bid_price = 4;
      tob.ask_price = 5;
    }
    bk_to_strat_buffer1.push_back(tob);

    // call strategy members located with component_main_loop
    arb_trader.handle_update();
    arb_trader.signal();
    arb_trader.execution();
    arb_trader.handle_response();

    arb_trader.handle_update();
    arb_trader.signal();
    arb_trader.execution();
    arb_trader.handle_response();

    // stand in order manager that says every order gets filled
    while (!strat_to_om_buffer_.empty())
    {
      to_gateway_out_t to_gateway_out = strat_to_om_buffer_.pop_front();
      to_gateway_out_t to_gateway_out1 = strat_to_om_buffer_.pop_front();

      from_gateway_out_t from_gateway_out{};
      from_gateway_out.status = status_type::filled;
      from_gateway_out.side = to_gateway_out.side;
      from_gateway_out.symbol = to_gateway_out.symbol;
      from_gateway_out.venue = to_gateway_out.venue;
      from_gateway_out.price = to_gateway_out.price;
      from_gateway_out.quantity = to_gateway_out.quantity;
      from_gateway_out.internal_order_id = to_gateway_out.internal_order_id;

      from_gateway_out_t from_gateway_out1 = from_gateway_out;
      from_gateway_out1.side = to_gateway_out1.side;
      from_gateway_out1.symbol = to_gateway_out1.symbol;
      from_gateway_out1.venue = to_gateway_out1.venue;
      from_gateway_out1.price = to_gateway_out1.price;
      from_gateway_out1.internal_order_id = to_gateway_out1.internal_order_id;

      om_to_strat_buffer_.push_back(from_gateway_out);
      om_to_strat_buffer_.push_back(from_gateway_out1);
    }
  }
  REQUIRE(arb_trader.current_position(symbol) == 0);

  REQUIRE(arb_trader.current_pnl(symbol) ==
            profit_per_trade * quantity * trade_opportunity_count);
}

TEST_CASE("test exit position when arbitrage trader's current unrealized pnl is"
            "below the stop loss", "[exit_stop_loss]")
{
  using namespace aligned;
  // will register three different books
  static bk_to_strat_buffer bk_to_strat_buffer1;
  static bk_to_strat_buffer bk_to_strat_buffer2;

  static om_to_strat_buffer om_to_strat_buffer_;
  static strat_to_om_buffer strat_to_om_buffer_;

  aligned_t arb_trader_id = 5;
  arbitrage_trader arb_trader(arb_trader_id,
                              om_to_strat_buffer_,
                              strat_to_om_buffer_);

  arb_trader.register_book_buffer(bk_to_strat_buffer1);
  arb_trader.register_book_buffer(bk_to_strat_buffer2);

  // using 2 different venues and one symbol
  auto symbol = symbol_name::symbol_four; // arbitrarily chosen
  auto venue1 = venue_name::venue_one; // arbitrarily chosen
  auto venue2 = venue_name::venue_two; // arbitrarily chosen
  arb_trader.register_market(symbol, venue1);
  arb_trader.register_market(symbol, venue2);

  aligned_t limit = 4;
  const aligned_t quantity = 5;
  const aligned_t cleanup_cycle = 0;
  unsigned trade_opportunity_count = 0;
  unsigned profit_per_trade = 1;
  for (aligned_t j = 0; j < limit + cleanup_cycle; ++j)
  {
    // put new tob into each buffer
    tob_t tob{};
    tob.venue = venue2;
    tob.bid_price = 26;
    tob.ask_price = 27;
    tob.bid_quantity = quantity;
    tob.ask_quantity = quantity;
    tob.symbol = symbol;
    bk_to_strat_buffer2.push_back(tob);
    tob.venue = venue1;
    if (j > 1 && j < 10 && j % 2 == 0)
    {
      ++trade_opportunity_count;
      tob.bid_price = 24;
      tob.ask_price = 25;
    }
    bk_to_strat_buffer1.push_back(tob);

    // call strategy members located with component_main_loop
    arb_trader.handle_update();
    arb_trader.signal();
    arb_trader.execution();
    arb_trader.handle_response();

    arb_trader.handle_update();
    arb_trader.signal();
    arb_trader.execution();
    arb_trader.handle_response();


    // stand in order manager that says every order gets filled
    while (!strat_to_om_buffer_.empty())
    {
      to_gateway_out_t to_gateway_out = strat_to_om_buffer_.pop_front();
      to_gateway_out_t to_gateway_out1 = strat_to_om_buffer_.pop_front();

      from_gateway_out_t from_gateway_out{};
      from_gateway_out.status = status_type::filled;
      from_gateway_out.side = to_gateway_out.side;
      from_gateway_out.symbol = to_gateway_out.symbol;
      from_gateway_out.venue = to_gateway_out.venue;
      from_gateway_out.price = to_gateway_out.price;
      from_gateway_out.quantity = to_gateway_out.quantity;
      from_gateway_out.internal_order_id = to_gateway_out.internal_order_id;

      from_gateway_out_t from_gateway_out1 = from_gateway_out;
      from_gateway_out1.side = to_gateway_out1.side;
      from_gateway_out1.symbol = to_gateway_out1.symbol;
      from_gateway_out1.venue = to_gateway_out1.venue;
      from_gateway_out1.price = to_gateway_out1.price;
      from_gateway_out1.internal_order_id = to_gateway_out1.internal_order_id;
      if (j == 2)
        from_gateway_out1.quantity = 1;

      om_to_strat_buffer_.push_back(from_gateway_out);
      om_to_strat_buffer_.push_back(from_gateway_out1);
    }
    auto position = arb_trader.current_position(symbol);
    if (j == 3)
      REQUIRE(position == 4);
  }
  arb_trader.cease_new_positions();
  // put new tob into each buffer
  tob_t tob{};
  tob.venue = venue2;
  tob.bid_price = 19;
  tob.ask_price = 20;
  tob.bid_quantity = quantity;
  tob.ask_quantity = quantity;
  tob.symbol = symbol;
  bk_to_strat_buffer2.push_back(tob);
  tob.venue = venue1;
  bk_to_strat_buffer1.push_back(tob);

  // call strategy members located with component_main_loop
  arb_trader.handle_update();
  arb_trader.signal();
  arb_trader.execution();
  arb_trader.handle_response();

  arb_trader.handle_update();
  arb_trader.signal();
  arb_trader.execution();
  arb_trader.handle_response();

  auto cur_ur_pnl = arb_trader.current_unrealized_pnl(symbol);
  REQUIRE(cur_ur_pnl < 0);
  REQUIRE(!strat_to_om_buffer_.empty());
  to_gateway_out_t to_gateway_out = strat_to_om_buffer_.pop_front();

  from_gateway_out_t from_gateway_out{};
  from_gateway_out.status = status_type::filled;
  from_gateway_out.side = to_gateway_out.side;
  from_gateway_out.symbol = to_gateway_out.symbol;
  from_gateway_out.venue = to_gateway_out.venue;
  from_gateway_out.price = 15;
  from_gateway_out.quantity = to_gateway_out.quantity;
  from_gateway_out.internal_order_id = to_gateway_out.internal_order_id;
  om_to_strat_buffer_.push_back(from_gateway_out);

  arb_trader.handle_update();
  arb_trader.signal();
  arb_trader.execution();
  arb_trader.handle_response();

  auto position = arb_trader.current_position(symbol);
  REQUIRE(position == 0);
  auto pnl = arb_trader.current_pnl(symbol);
  REQUIRE(pnl == - 39);

  auto ttl_quotes = arb_trader.total_quantity_quoting();
  REQUIRE(ttl_quotes == 0);
}

TEST_CASE("test viewer database for strategies orders" , "[strat_order_db]")
{
  using namespace aligned;
  // will register three different books
  static bk_to_strat_buffer bk_to_strat_buffer1;
  static bk_to_strat_buffer bk_to_strat_buffer2;

  static om_to_strat_buffer om_to_strat_buffer_;
  static strat_to_om_buffer strat_to_om_buffer_;

  aligned_t arb_trader_id = 5;
  arbitrage_trader arb_trader(arb_trader_id,
                              om_to_strat_buffer_,
                              strat_to_om_buffer_);

  arb_trader.register_book_buffer(bk_to_strat_buffer1);
  arb_trader.register_book_buffer(bk_to_strat_buffer2);

  // using 2 different venues and one symbol
  auto symbol = symbol_name::symbol_four; // arbitrarily chosen
  auto venue1 = venue_name::venue_one; // arbitrarily chosen
  auto venue2 = venue_name::venue_two; // arbitrarily chosen
  arb_trader.register_market(symbol, venue1);
  arb_trader.register_market(symbol, venue2);

  REQUIRE(arb_trader.is_ready());

  aligned_t id = 71;
  aligned_t px = 31;
  aligned_t qty = 20;
  auto type = order_type::limit;
  auto side = side_type::bid_side;
  auto sym = symbol_name::symbol_one;
  auto ven = venue_name::venue_two;

  bid b{};
  ask a{};

  b.order_id = id;
  b.price = px;
  b.quantity = qty;
  b.order_type = type;
  b.symbol = sym;
  b.venue = ven;
  b.side = side;

  a.order_id = id+1;
  a.price = px + 2;
  a.quantity = qty;
  a.order_type = type;
  a.symbol = sym;
  a.venue = ven;
  a.side = side_type::ask_side;

  arb_trader.add_quote_database(b);

  arb_trader.add_quote_database(a);

  b.quantity = b.quantity / 2;
  arb_trader.update_quote_database(b);

  a.quantity = 4;
  arb_trader.update_quote_database(a);

  b.quantity = 0;
  arb_trader.update_quote_database(b);

  a.quantity = 0;
  arb_trader.update_quote_database(a);

  arb_trader.drop_all_orders();
}




#pragma clang diagnostic pop
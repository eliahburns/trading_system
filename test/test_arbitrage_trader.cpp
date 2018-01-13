//
// Created by eli on 8/14/17.
//

#include "../arbitrage_trader.hpp"
#include "catch.hpp"



#pragma clang diagnostic push
#pragma ide diagnostic ignored "ClangTidyInspection"

TEST_CASE("test arbitrage trader constructor", "[arb_trader_constructor]")
{
  using namespace aligned;
  static bk_to_strat_buffer bk_to_strat_buffer1;
  static om_to_strat_buffer om_to_strat_buffer_;
  static strat_to_om_buffer strat_to_om_buffer_;

  aligned_t arb_trdr_id = 5;

  arbitrage_trader arb_trader(arb_trdr_id,
                              om_to_strat_buffer_,
                              strat_to_om_buffer_);

  arb_trader.register_book_buffer(bk_to_strat_buffer1);
}

TEST_CASE("test adding symbol, start position", "[add_symbol_start_pos]")
{
  using namespace aligned;
  static bk_to_strat_buffer bk_to_strat_buffer1;
  static om_to_strat_buffer om_to_strat_buffer_;
  static strat_to_om_buffer strat_to_om_buffer_;
  aligned_t arb_trdr_id = 5;
  arbitrage_trader arb_trader(arb_trdr_id,
                              om_to_strat_buffer_,
                              strat_to_om_buffer_);

  arb_trader.register_book_buffer(bk_to_strat_buffer1);

  auto symbol = aligned::symbol_name::symbol_two;
  auto venue = aligned::venue_name::venue_three;
  arb_trader.register_market(symbol, venue);

  auto start_position = arb_trader.current_position(symbol);

  REQUIRE(start_position == 0);
}

TEST_CASE("update position for symbol and check pnl", "[update_pos_pnl]")
{
  using namespace aligned;
  static bk_to_strat_buffer bk_to_strat_buffer1;
  static om_to_strat_buffer om_to_strat_buffer_;
  static strat_to_om_buffer strat_to_om_buffer_;
  aligned_t arb_trdr_id = 5;
  arbitrage_trader arb_trader(arb_trdr_id,
                              om_to_strat_buffer_,
                              strat_to_om_buffer_);

  arb_trader.register_book_buffer(bk_to_strat_buffer1);

  auto symbol = aligned::symbol_name::symbol_two;
  auto venue = aligned::venue_name::venue_three;
  arb_trader.register_market(symbol, venue);

  auto start_position = arb_trader.current_position(symbol);
  REQUIRE(start_position == 0);

  auto start_side = aligned::side_type::bid_side;
  auto end_side = aligned::side_type::ask_side;

  aligned_t start_px = 15, end_px = 17;
  aligned_t start_qty = 2, end_qty = 2;

  arb_trader.update_position(symbol, start_side, start_px, start_qty);

  auto cur_pos = arb_trader.current_position(symbol);
  REQUIRE(cur_pos == start_qty);

  arb_trader.update_position(symbol, end_side, end_px, end_qty);
  cur_pos = arb_trader.current_position(symbol);
  REQUIRE(cur_pos == start_qty - end_qty);

  // should be profitable
  double cur_pnl = arb_trader.current_pnl(symbol);
  REQUIRE(cur_pnl == (end_px - start_px)*start_qty);

  arb_trader.reset_realized_pnl(symbol);
  // another round with different prices and ending in loss
  start_px = 32, end_px = 29;
  start_qty = end_qty = 4;

  arb_trader.update_position(symbol, start_side, start_px, start_qty);

  cur_pos = arb_trader.current_position(symbol);
  REQUIRE(cur_pos == start_qty);

  arb_trader.update_position(symbol, end_side, end_px, end_qty);
  cur_pos = arb_trader.current_position(symbol);
  REQUIRE(cur_pos == start_qty - end_qty);

  // should be profitable
  cur_pnl = arb_trader.current_pnl(symbol);
  REQUIRE(cur_pnl == (29-32)*4);
}

TEST_CASE("test correct pnl after closing position of initially short",
          "[pnl_after_closing_short]")
{
  using namespace aligned;
  static bk_to_strat_buffer bk_to_strat_buffer1;
  static om_to_strat_buffer om_to_strat_buffer_;
  static strat_to_om_buffer strat_to_om_buffer_;
  aligned_t arb_trader_id = 5;
  arbitrage_trader arb_trader(arb_trader_id,
                              om_to_strat_buffer_,
                              strat_to_om_buffer_);

  arb_trader.register_book_buffer(bk_to_strat_buffer1);

  auto symbol = symbol_name::symbol_two;
  auto venue = venue_name::venue_three;
  arb_trader.register_market(symbol, venue);

  auto start_position = arb_trader.current_position(symbol);
  REQUIRE(start_position == 0);

  auto side_start = side_type::ask_side;
  auto side_end = side_type::bid_side;
  double scalar = (side_start == side_type::ask_side) ? -1 : 1;

  {
    // expect pnl at end to be profitable
    aligned_t px_start = 27, px_end = 23;
    aligned_t qty = 5;
    double pxs = px_start, pxe = px_end, q = qty;

    // sell price @ quantity
    arb_trader.update_position(symbol, side_start, px_start, qty);
    auto cur_pos = arb_trader.current_position(symbol);
    REQUIRE(cur_pos == -q);

    // buy price @ quantity
    arb_trader.update_position(symbol, side_end, px_end, qty);
    cur_pos = arb_trader.current_position(symbol);
    REQUIRE(cur_pos == q - q);

    auto cur_pnl = arb_trader.current_pnl(symbol);
    REQUIRE(cur_pnl == (pxe - pxs)*q*scalar);
    REQUIRE(cur_pnl > 0);
  }
  arb_trader.reset_realized_pnl(symbol);
  {
    // expect pnl at end to be unprofitable
    aligned_t px_start = 27, px_end = 31;
    aligned_t qty = 3;
    double pxs = px_start, pxe = px_end, q = qty;

    // sell price @ quantity
    arb_trader.update_position(symbol, side_start, px_start, qty);
    auto cur_pos = arb_trader.current_position(symbol);
    REQUIRE(cur_pos == -q);

    // buy price @ quantity
    arb_trader.update_position(symbol, side_end, px_end, qty);
    cur_pos = arb_trader.current_position(symbol);
    REQUIRE(cur_pos == q - q);

    auto cur_pnl = arb_trader.current_pnl(symbol);
    REQUIRE(cur_pnl == (pxe - pxs)*q*scalar);
    REQUIRE(cur_pnl < 0);
  }
  arb_trader.reset_realized_pnl(symbol);
  {
    // expect pnl at end to be ZERO
    aligned_t px_start = 27, px_end = 27;
    aligned_t qty = 7;
    double pxs = px_start, pxe = px_end, q = qty;

    // sell price @ quantity
    arb_trader.update_position(symbol, side_start, px_start, qty);
    auto cur_pos = arb_trader.current_position(symbol);
    REQUIRE(cur_pos == -q);

    // buy price @ quantity
    arb_trader.update_position(symbol, side_end, px_end, qty);
    cur_pos = arb_trader.current_position(symbol);
    REQUIRE(cur_pos == q - q);

    auto cur_pnl = arb_trader.current_pnl(symbol);
    REQUIRE(cur_pnl == (pxe - pxs)*q*scalar);
    REQUIRE(cur_pnl == 0);
  }
}

TEST_CASE("test current pnl while in position", "[current_pnl]")
{
  using namespace aligned;
  static bk_to_strat_buffer bk_to_strat_buffer1;
  static om_to_strat_buffer om_to_strat_buffer_;
  static strat_to_om_buffer strat_to_om_buffer_;
  aligned_t arb_trader_id = 5;
  arbitrage_trader arb_trader(arb_trader_id,
                              om_to_strat_buffer_,
                              strat_to_om_buffer_);

  arb_trader.register_book_buffer(bk_to_strat_buffer1);

  auto symbol = symbol_name::symbol_two;
  auto venue = venue_name::venue_three;
  arb_trader.register_market(symbol, venue);

  auto start_position = arb_trader.current_position(symbol);
  REQUIRE(start_position == 0);

  auto side_start = side_type::bid_side;
  auto side_end = side_type::ask_side;
  double scalar = (side_start == side_type::ask_side) ? -1 : 1;

  // need to add a fake tob of book for use in getting current pnl
  // while in position
  tob_t tob{};

  tob.symbol = symbol;
  tob.venue = venue;
  tob.bid_price = 22;
  tob.ask_price = 23;
  tob.bid_quantity = 3;
  tob.ask_quantity = 5;

  arb_trader.update_symbol_tob_market(tob);
  {
    // expect pnl at end to be profitable
    aligned_t px_start = 23, px_end = 27;
    aligned_t qty = 5;
    double pxs = px_start, pxe = px_end, q = qty;

    // buy price @ quantity
    arb_trader.update_position(symbol, side_start, px_start, qty);
    auto cur_pos = arb_trader.current_position(symbol);
    REQUIRE(cur_pos == scalar * q);

    auto cur_pnl = arb_trader.current_pnl(symbol);
    REQUIRE(cur_pnl < 0);
    double theo_px = (22.0*3 + 23.0*5) / (3 + 5);
    double exp_pnl = (theo_px - px_start) * qty;
    REQUIRE(cur_pnl == exp_pnl);

    // sell price @ quantity
    arb_trader.update_position(symbol, side_end, px_end, qty);
    cur_pos = arb_trader.current_position(symbol);
    REQUIRE(cur_pos == q - q);

    cur_pnl = arb_trader.current_pnl(symbol);
    REQUIRE(cur_pnl == (pxe - pxs) * q * scalar);
    REQUIRE(cur_pnl > 0);
  }
}

TEST_CASE("test current pnl while in position with symbol in more than one "
            "market (in the money)", "[current_pnl_ITM]")
{
  using namespace aligned;
  static bk_to_strat_buffer bk_to_strat_buffer1;
  static om_to_strat_buffer om_to_strat_buffer_;
  static strat_to_om_buffer strat_to_om_buffer_;
  aligned_t arb_trader_id = 5;
  arbitrage_trader arb_trader(arb_trader_id,
                              om_to_strat_buffer_,
                              strat_to_om_buffer_);

  arb_trader.register_book_buffer(bk_to_strat_buffer1);

  auto symbol = symbol_name::symbol_two; // arbitrarily chosen
  auto venue = venue_name::venue_three; // arbitrarily chosen
  auto venue2 = venue_name::venue_four; // arbitrarily chosen
  // register markets at different venues for the same symbol
  arb_trader.register_market(symbol, venue);
  arb_trader.register_market(symbol, venue2);

  auto start_position = arb_trader.current_position(symbol);
  REQUIRE(start_position == 0);

  auto side_start = side_type::bid_side;
  auto side_end = side_type::ask_side;
  double scalar = (side_start == side_type::ask_side) ? -1 : 1;

  // We need to add a fake top of book for use in getting current pnl
  // while in position, for each market.
  tob_t tob{};
  tob.symbol = symbol;
  tob.venue = venue;
  tob.bid_price = 22;
  tob.ask_price = 23;
  tob.bid_quantity = 3;
  tob.ask_quantity = 5;

  tob_t tob2{};
  tob2.symbol = symbol;
  tob2.venue = venue2;
  tob2.bid_price = 27;
  tob2.ask_price = 29;
  tob2.bid_quantity = 7;
  tob2.ask_quantity = 11;

  // update the markets within the strategy
  arb_trader.update_symbol_tob_market(tob);
  arb_trader.update_symbol_tob_market(tob2);
  {
    // expect pnl at end to be profitable
    aligned_t px_start = 23, px_end = 27;
    aligned_t qty = 5;
    double pxs = to_type<double>(px_start);
    double pxe = to_type<double>(px_end);
    double q = to_type<double>(qty);
    double bid_mkt2 = to_type<double>(tob2.bid_price);
    double ask_mkt2 = to_type<double>(tob2.ask_price);
    double bid_qty_mkt2 = to_type<double>(tob2.bid_quantity);
    double ask_qty_mkt2 = to_type<double>(tob2.ask_quantity);

    // buy price @ quantity
    arb_trader.update_position(symbol, side_start, px_start, qty);
    auto cur_pos = arb_trader.current_position(symbol);
    REQUIRE(cur_pos == scalar * q);

    auto cur_pnl = arb_trader.current_pnl(symbol);
    // theoretical price to exit will come from the market corresponding to
    // tob2.
    double theo_px = (bid_mkt2*bid_qty_mkt2 + ask_mkt2*ask_qty_mkt2) /
      (bid_qty_mkt2 + ask_qty_mkt2);
    double exp_pnl = (theo_px - px_start) * qty;
    double epsilon = 0.00001;
    REQUIRE((cur_pnl - exp_pnl) < epsilon);

    // sell price @ quantity
    arb_trader.update_position(symbol, side_end, px_end, qty);
    cur_pos = arb_trader.current_position(symbol);
    REQUIRE(cur_pos == q - q);

    cur_pnl = arb_trader.current_pnl(symbol);
    REQUIRE(cur_pnl == (pxe - pxs) * q * scalar);
    REQUIRE(cur_pnl > 0);
  }
}

TEST_CASE("test current pnl while in position with symbol in more than one "
            "market (out of the money)", "[current_pnl_OTM]")
{
  using namespace aligned;
  static bk_to_strat_buffer bk_to_strat_buffer1;
  static om_to_strat_buffer om_to_strat_buffer_;
  static strat_to_om_buffer strat_to_om_buffer_;
  aligned_t arb_trader_id = 5;
  arbitrage_trader arb_trader(arb_trader_id,
                              om_to_strat_buffer_,
                              strat_to_om_buffer_);

  arb_trader.register_book_buffer(bk_to_strat_buffer1);

  auto symbol = symbol_name::symbol_two; // arbitrarily chosen
  auto venue = venue_name::venue_three; // arbitrarily chosen
  auto venue2 = venue_name::venue_four; // arbitrarily chosen
  // register markets at different venues for the same symbol
  arb_trader.register_market(symbol, venue);
  arb_trader.register_market(symbol, venue2);

  auto start_position = arb_trader.current_position(symbol);
  REQUIRE(start_position == 0);

  auto side_start = side_type::bid_side;
  auto side_end = side_type::ask_side;
  double scalar = (side_start == side_type::ask_side) ? -1 : 1;

  // We need to add a fake top of book for use in getting current pnl
  // while in position, for each market.
  tob_t tob{};
  tob.symbol = symbol;
  tob.venue = venue;
  tob.bid_price = 22;
  tob.ask_price = 23;
  tob.bid_quantity = 9;
  tob.ask_quantity = 7;

  tob_t tob2{};
  tob2.symbol = symbol;
  tob2.venue = venue2;
  tob2.bid_price = 19;
  tob2.ask_price = 21;
  tob2.bid_quantity = 7;
  tob2.ask_quantity = 11;

  // update the markets within the strategy
  arb_trader.update_symbol_tob_market(tob);
  arb_trader.update_symbol_tob_market(tob2);
  {
    // Expect pnl at end to be unprofitable.
    // In position pnl should now be derived by the market corresponding
    // to tob, since exiting there would be financially optimal.
    aligned_t px_start = 23, px_end = 20;
    aligned_t qty = 23;
    double pxs = to_type<double>(px_start);
    double pxe = to_type<double>(px_end);
    double q = to_type<double>(qty);
    double bid_mkt = to_type<double>(tob.bid_price);
    double ask_mkt = to_type<double>(tob.ask_price);
    double bid_qty_mkt = to_type<double>(tob.bid_quantity);
    double ask_qty_mkt = to_type<double>(tob.ask_quantity);

    // buy price @ quantity
    arb_trader.update_position(symbol, side_start, px_start, qty);
    auto cur_pos = arb_trader.current_position(symbol);
    REQUIRE(cur_pos == scalar * q);

    auto cur_pnl = arb_trader.current_pnl(symbol);
    // theoretical price to exit will come from the market corresponding to
    // tob2.
    double theo_px = (bid_mkt*bid_qty_mkt + ask_mkt*ask_qty_mkt) /
                     (bid_qty_mkt + ask_qty_mkt);
    double exp_pnl = (theo_px - px_start) * qty;
    double epsilon = 0.00001;
    REQUIRE((cur_pnl - exp_pnl) < epsilon);

    // sell price @ quantity
    arb_trader.update_position(symbol, side_end, px_end, qty);
    cur_pos = arb_trader.current_position(symbol);
    REQUIRE(cur_pos == q - q);

    cur_pnl = arb_trader.current_pnl(symbol);
    REQUIRE(cur_pnl == (pxe - pxs) * q * scalar);
  }
}

TEST_CASE("test ending pnl after accumulating bids at more than one price",
          "[ending_pnl_position_multiple_px]")
{
  using namespace aligned;
  static bk_to_strat_buffer bk_to_strat_buffer1;
  static om_to_strat_buffer om_to_strat_buffer_;
  static strat_to_om_buffer strat_to_om_buffer_;
  aligned_t arb_trader_id = 5;
  arbitrage_trader arb_trader(arb_trader_id,
                              om_to_strat_buffer_,
                              strat_to_om_buffer_);

  arb_trader.register_book_buffer(bk_to_strat_buffer1);

  auto symbol = symbol_name::symbol_two; // arbitrarily chosen
  auto venue = venue_name::venue_three; // arbitrarily chosen
  auto venue2 = venue_name::venue_four; // arbitrarily chosen
  // register markets at different venues for the same symbol
  arb_trader.register_market(symbol, venue);
  arb_trader.register_market(symbol, venue2);

  auto start_position = arb_trader.current_position(symbol);
  REQUIRE(start_position == 0);

  auto side_start = side_type::bid_side;
  auto side_end = side_type::ask_side;
  double scalar = (side_start == side_type::ask_side) ? -1 : 1;

  // We need to add a fake top of book for use in getting current pnl
  // while in position, for each market.
  tob_t tob{};
  tob.symbol = symbol;
  tob.venue = venue;
  tob.bid_price = 22;
  tob.ask_price = 23;
  tob.bid_quantity = 9;
  tob.ask_quantity = 7;

  tob_t tob2{};
  tob2.symbol = symbol;
  tob2.venue = venue2;
  tob2.bid_price = 19;
  tob2.ask_price = 21;
  tob2.bid_quantity = 7;
  tob2.ask_quantity = 11;

  // update the markets within the strategy
  arb_trader.update_symbol_tob_market(tob);
  arb_trader.update_symbol_tob_market(tob2);
  {
    // Expect pnl at end to be unprofitable.
    // In position pnl should now be derived by the market corresponding
    // to tob, since exiting there would be financially optimal.
    aligned_t px_start = 23, px_end = 20;
    aligned_t qty = 23;
    double pxs = to_type<double>(px_start);
    double pxe = to_type<double>(px_end);
    double q = to_type<double>(qty);
    double bid_mkt = to_type<double>(tob.bid_price);
    double ask_mkt = to_type<double>(tob.ask_price);
    double bid_qty_mkt = to_type<double>(tob.bid_quantity);
    double ask_qty_mkt = to_type<double>(tob.ask_quantity);

    // buy price @ quantity
    arb_trader.update_position(symbol, side_start, px_start, qty);
    auto cur_pos = arb_trader.current_position(symbol);
    REQUIRE(cur_pos == scalar * q);

    auto cur_pnl = arb_trader.current_pnl(symbol);
    // theoretical price to exit will come from the market corresponding to
    // tob2.
    double theo_px = (bid_mkt*bid_qty_mkt + ask_mkt*ask_qty_mkt) /
                     (bid_qty_mkt + ask_qty_mkt);
    double exp_pnl = (theo_px - px_start) * qty * scalar;
    double epsilon = 0.00001;
    REQUIRE((cur_pnl - exp_pnl) < epsilon);

    // buy price @ quantity (DOUBLE POSITION AT SAME PRICE)
    arb_trader.update_position(symbol, side_start, px_start, qty);
    cur_pnl = arb_trader.current_pnl(symbol);
    exp_pnl = (theo_px - px_start) * 2 * qty * scalar;
    REQUIRE((cur_pnl - exp_pnl) < epsilon);

    // sell price @ quantity
    arb_trader.update_position(symbol, side_end, px_end, qty);
    cur_pos = arb_trader.current_position(symbol);
    REQUIRE(cur_pos == q);

    // sell price @ quantity
    arb_trader.update_position(symbol, side_end, px_end, qty);
    cur_pos = arb_trader.current_position(symbol);
    REQUIRE(cur_pos == q - q);

    cur_pnl = arb_trader.current_pnl(symbol);
    REQUIRE(cur_pnl == (pxe - pxs) * 2 * q * scalar);
  }
}

TEST_CASE("test ending pnl after accumulating asks at more than one price",
          "[ending_pnl_position_multiple_px_asks_init]")
{
  using namespace aligned;
  static bk_to_strat_buffer bk_to_strat_buffer1;
  static om_to_strat_buffer om_to_strat_buffer_;
  static strat_to_om_buffer strat_to_om_buffer_;
  aligned_t arb_trader_id = 5;
  arbitrage_trader arb_trader(arb_trader_id,
                              om_to_strat_buffer_,
                              strat_to_om_buffer_);

  arb_trader.register_book_buffer(bk_to_strat_buffer1);

  auto symbol = symbol_name::symbol_two; // arbitrarily chosen
  auto venue = venue_name::venue_three; // arbitrarily chosen
  auto venue2 = venue_name::venue_four; // arbitrarily chosen
  // register markets at different venues for the same symbol
  arb_trader.register_market(symbol, venue);
  arb_trader.register_market(symbol, venue2);

  auto start_position = arb_trader.current_position(symbol);
  REQUIRE(start_position == 0);

  auto side_start = side_type::ask_side;
  auto side_end = side_type::bid_side;
  double scalar = (side_start == side_type::ask_side) ? -1 : 1;

  // We need to add a fake top of book for use in getting current pnl
  // while in position, for each market.
  tob_t tob{};
  tob.symbol = symbol;
  tob.venue = venue;
  tob.bid_price = 22;
  tob.ask_price = 23;
  tob.bid_quantity = 9;
  tob.ask_quantity = 7;

  tob_t tob2{};
  tob2.symbol = symbol;
  tob2.venue = venue2;
  tob2.bid_price = 19;
  tob2.ask_price = 21;
  tob2.bid_quantity = 7;
  tob2.ask_quantity = 11;

  // update the markets within the strategy
  arb_trader.update_symbol_tob_market(tob);
  arb_trader.update_symbol_tob_market(tob2);
  {
    // Expect pnl at end to be profitable.
    // In position pnl should now be derived by the market corresponding
    // to tob, since exiting there would be financially optimal.
    aligned_t px_start = 22, px_end = 19;
    aligned_t qty = 3;
    double q = to_type<double>(qty);
    double bid_mkt = to_type<double>(tob2.bid_price);
    double ask_mkt = to_type<double>(tob2.ask_price);
    double bid_qty_mkt = to_type<double>(tob2.bid_quantity);
    double ask_qty_mkt = to_type<double>(tob2.ask_quantity);

    // sell price @ quantity
    arb_trader.update_position(symbol, side_start, px_start, qty);
    auto cur_pos = arb_trader.current_position(symbol);
    REQUIRE(cur_pos == scalar * q);

    auto cur_pnl = arb_trader.current_pnl(symbol);
    // theoretical price to exit will come from the market corresponding to
    // tob2.
    double theo_px = (bid_mkt*bid_qty_mkt + ask_mkt*ask_qty_mkt) /
                     (bid_qty_mkt + ask_qty_mkt);
    double exp_pnl = (theo_px - px_start) * qty * scalar;
    double epsilon = 0.00001;
    double actual_difference = cur_pnl - exp_pnl;
    REQUIRE(actual_difference < epsilon);

    // sell price @ quantity
    double px_start2 = px_start+1;
    double qty2 = qty-1;
    arb_trader.update_position(symbol, side_start, px_start2, qty2);
    cur_pos = arb_trader.current_position(symbol);
    REQUIRE(cur_pos ==  scalar * (q+2));
    // theoretical price hasn't changed, but now we must weight the entry px
    double weighted_px_start = (px_start*qty + px_start2*qty2) / (qty+qty2);
    double exp_pnl2 = (theo_px - weighted_px_start) * qty * scalar;
    double actual_difference2 = cur_pnl - exp_pnl2;
    REQUIRE(actual_difference2 < epsilon);

    // buy price @ quantity
    arb_trader.update_position(symbol, side_end, px_end, qty);
    cur_pos = arb_trader.current_position(symbol);
    REQUIRE(cur_pos == (q-1) * scalar);
    // buy price @ quantity
    arb_trader.update_position(symbol, side_end, px_end, qty-1);
    cur_pos = arb_trader.current_position(symbol);
    REQUIRE(cur_pos == (2*q-1) - (2*q-1));

    cur_pnl = arb_trader.current_pnl(symbol);
    double exp_pnl_final = (px_end - weighted_px_start) * (2*q-1) * scalar;
    REQUIRE((cur_pnl - exp_pnl_final) < epsilon);
  }
}

TEST_CASE("test total markets registered", "[markets_registered]")
{
  using namespace aligned;
  static bk_to_strat_buffer bk_to_strat_buffer1;
  static om_to_strat_buffer om_to_strat_buffer_;
  static strat_to_om_buffer strat_to_om_buffer_;
  aligned_t arb_trader_id = 5;
  arbitrage_trader arb_trader(arb_trader_id,
                              om_to_strat_buffer_,
                              strat_to_om_buffer_);

  arb_trader.register_book_buffer(bk_to_strat_buffer1);

  auto symbol = symbol_name::symbol_two; // arbitrarily chosen
  auto venue = venue_name::venue_three; // arbitrarily chosen
  auto venue2 = venue_name::venue_four; // arbitrarily chosen
  auto venue3 = venue_name::venue_two; // arbitrarily chosen
  auto venue4 = venue_name::venue_one; // arbitrarily chosen
  // register markets at different venues for the same symbol
  aligned_t ttl_mkts = 0;
  aligned_t exp_mkts = 0;
  arb_trader.register_market(symbol, venue);
  ttl_mkts = arb_trader.total_markets_registered(symbol);
  REQUIRE(ttl_mkts == ++exp_mkts);
  arb_trader.register_market(symbol, venue2);
  ttl_mkts = arb_trader.total_markets_registered(symbol);
  REQUIRE(ttl_mkts == ++exp_mkts);
  arb_trader.register_market(symbol, venue3);
  ttl_mkts = arb_trader.total_markets_registered(symbol);
  REQUIRE(ttl_mkts == ++exp_mkts);
  arb_trader.register_market(symbol, venue4);
  ttl_mkts = arb_trader.total_markets_registered(symbol);
  REQUIRE(ttl_mkts == ++exp_mkts);
}

// testing arbitrage trader specifically now
TEST_CASE("test handle update : arbitrage trader", "[handle_update_arb_trdr]")
{
  using namespace aligned;
  // will register three different books
  static bk_to_strat_buffer bk_to_strat_buffer1;
  static bk_to_strat_buffer bk_to_strat_buffer2;
  static bk_to_strat_buffer bk_to_strat_buffer3;

  static om_to_strat_buffer om_to_strat_buffer_;
  static strat_to_om_buffer strat_to_om_buffer_;
  aligned_t arb_trader_id = 5;
  arbitrage_trader arb_trader(arb_trader_id,
                              om_to_strat_buffer_,
                              strat_to_om_buffer_);

  arb_trader.register_book_buffer(bk_to_strat_buffer1);
  arb_trader.register_book_buffer(bk_to_strat_buffer2);
  arb_trader.register_book_buffer(bk_to_strat_buffer3);

  // create of top of book object and put it into bk_to_strat_buffer
  tob_t tob{};
  bk_to_strat_buffer1.push_back(tob);
  bk_to_strat_buffer2.push_back(tob);
  bk_to_strat_buffer3.push_back(tob);
  // require that all buffers are NOT empty
  REQUIRE_FALSE(bk_to_strat_buffer1.empty());
  REQUIRE_FALSE(bk_to_strat_buffer2.empty());
  REQUIRE_FALSE(bk_to_strat_buffer3.empty());
  arb_trader.handle_update();
  // require that all buffers are empty after call to handle update
  REQUIRE(bk_to_strat_buffer1.empty());
  REQUIRE(bk_to_strat_buffer2.empty());
  REQUIRE(bk_to_strat_buffer3.empty());
}

TEST_CASE("test all markets available : arbitrage trader", "[all_mkts_ready]")
{
  using namespace aligned;
  // will register three different books
  static bk_to_strat_buffer bk_to_strat_buffer1;
  static bk_to_strat_buffer bk_to_strat_buffer2;
  static bk_to_strat_buffer bk_to_strat_buffer3;

  static om_to_strat_buffer om_to_strat_buffer_;
  static strat_to_om_buffer strat_to_om_buffer_;

  aligned_t arb_trader_id = 5;
  arbitrage_trader arb_trader(arb_trader_id,
                              om_to_strat_buffer_,
                              strat_to_om_buffer_);

  arb_trader.register_book_buffer(bk_to_strat_buffer1);
  arb_trader.register_book_buffer(bk_to_strat_buffer2);
  arb_trader.register_book_buffer(bk_to_strat_buffer3);

  // using 3 different venues and one symbol
  auto symbol = symbol_name::symbol_two; // arbitrarily chosen
  auto venue1 = venue_name::venue_one; // arbitrarily chosen
  auto venue2 = venue_name::venue_two; // arbitrarily chosen
  auto venue3 = venue_name::venue_three; // arbitrarily chosen

  arb_trader.register_market(symbol, venue1);
  arb_trader.register_market(symbol, venue2);
  arb_trader.register_market(symbol, venue3);
  REQUIRE_FALSE(arb_trader.all_markets_available());

  // add tob markets for each venue to corresponding buffer
  using bks_to_strat_vec =
  std::vector<std::reference_wrapper<bk_to_strat_buffer>>;
  bks_to_strat_vec bts_vec;
  bts_vec.push_back(
    std::reference_wrapper<bk_to_strat_buffer >(bk_to_strat_buffer1));
  bts_vec.push_back(
    std::reference_wrapper<bk_to_strat_buffer >(bk_to_strat_buffer2));
  bts_vec.push_back(
    std::reference_wrapper<bk_to_strat_buffer >(bk_to_strat_buffer3));
  std::vector<venue_name> v_names{};
  v_names.push_back(venue1);
  v_names.push_back(venue2);
  v_names.push_back(venue3);
  int i = -1;
  for (auto it = bts_vec.begin(); it != bts_vec.end(); ++it)
  {
    tob_t tob{};
    tob.venue = v_names[++i];
    tob.bid_price = 4;
    tob.ask_price = 5;
    tob.bid_quantity = 3;
    tob.ask_quantity = 7;
    tob.symbol = symbol;
    it->get().push_back(tob);
  }
  // this should loop only once and find all tob's needed
  arb_trader.wait_till_ready();
  REQUIRE(arb_trader.all_markets_available());
}

TEST_CASE("test signal : arbitrage trader", "[test_signal_arb_trdr]")
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

  // using 3 different venues and one symbol
  auto symbol = symbol_name::symbol_two; // arbitrarily chosen
  auto venue1 = venue_name::venue_one; // arbitrarily chosen
  auto venue2 = venue_name::venue_two; // arbitrarily chosen

  arb_trader.register_market(symbol, venue1);
  arb_trader.register_market(symbol, venue2);
  REQUIRE_FALSE(arb_trader.all_markets_available());

  // add tob markets for each venue to corresponding buffer
  using bks_to_strat_vec =
  std::vector<std::reference_wrapper<bk_to_strat_buffer>>;
  bks_to_strat_vec bts_vec;
  bts_vec.push_back(
    std::reference_wrapper<bk_to_strat_buffer >(bk_to_strat_buffer1));
  bts_vec.push_back(
    std::reference_wrapper<bk_to_strat_buffer >(bk_to_strat_buffer2));
  std::vector<venue_name> v_names{};
  v_names.push_back(venue1);
  v_names.push_back(venue2);
  int i = -1;
  for (auto it = bts_vec.begin(); it != bts_vec.end(); ++it)
  {
    tob_t tob{};
    tob.venue = v_names[++i];
    tob.bid_price = 6;
    tob.ask_price = 7;
    tob.bid_quantity = 21+to_type<aligned_t>(i);
    tob.ask_quantity = 19-to_type<aligned_t>(i);
    tob.symbol = symbol;
    it->get().push_back(tob);
  }
  // this should loop only once and find all tob's needed
  arb_trader.wait_till_ready();
  REQUIRE(arb_trader.all_markets_available());
  arb_trader.handle_update();
  REQUIRE(arb_trader.signal_buffer_size() == 0);
  arb_trader.signal();
  REQUIRE(arb_trader.signal_buffer_size() == 0);
  REQUIRE(arb_trader.execution_buffer_size() == 0);
  // update tob markets so there is an arbitrage opportunity
  {
    tob_t tob{};
    tob.bid_price = 6;
    tob.ask_price = 7;
    tob.bid_quantity = 17;
    tob.ask_quantity = 13;
    tob.symbol = symbol;
    tob.venue = venue1;
    bk_to_strat_buffer1.push_back(tob);
  }
  {
    tob_t tob{};
    tob.bid_price = 4;
    tob.ask_price = 5;
    tob.bid_quantity = 9;
    tob.ask_quantity = 11;
    tob.symbol = symbol;
    tob.venue = venue2;
    bk_to_strat_buffer2.push_back(tob);
  }
  // should funnel all updates into signal buffer
  arb_trader.handle_update();
  REQUIRE(arb_trader.signal_buffer_size() == 2);

  // after signal is called the second time, there should be two orders in the
  // execution buffer
  arb_trader.signal();
  arb_trader.signal();
  REQUIRE(arb_trader.execution_buffer_size() == 2);
  // check contents of execution buffer
  arbitrage_trader::execution_buffer& exec_buff =
    arb_trader.get_execution_buffer_reference();
  to_gateway_out_t to_gateway_out = exec_buff.pop_front();
  REQUIRE(to_gateway_out.side == side_type::bid_side);
  REQUIRE(to_gateway_out.symbol == symbol);
  REQUIRE(to_gateway_out.venue == venue2);
  REQUIRE(to_gateway_out.price == 5);
  REQUIRE(to_gateway_out.quantity == 11);

  to_gateway_out = exec_buff.pop_front();
  REQUIRE(to_gateway_out.side == side_type::ask_side);
  REQUIRE(to_gateway_out.symbol == symbol);
  REQUIRE(to_gateway_out.venue == venue1);
  REQUIRE(to_gateway_out.price == 6);
  REQUIRE(to_gateway_out.quantity == 11);
}

TEST_CASE("test signal : arbitrage trader (reversed condition)",
          "[test_signal_arb_trdr_reversed]")
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
  auto symbol = symbol_name::symbol_two; // arbitrarily chosen
  auto venue1 = venue_name::venue_one; // arbitrarily chosen
  auto venue2 = venue_name::venue_two; // arbitrarily chosen

  arb_trader.register_market(symbol, venue1);
  arb_trader.register_market(symbol, venue2);
  REQUIRE_FALSE(arb_trader.all_markets_available());

  // add tob markets for each venue to corresponding buffer
  using bks_to_strat_vec =
  std::vector<std::reference_wrapper<bk_to_strat_buffer>>;
  bks_to_strat_vec bts_vec;
  bts_vec.push_back(
    std::reference_wrapper<bk_to_strat_buffer >(bk_to_strat_buffer1));
  bts_vec.push_back(
    std::reference_wrapper<bk_to_strat_buffer >(bk_to_strat_buffer2));
  std::vector<venue_name> v_names{};
  v_names.push_back(venue1);
  v_names.push_back(venue2);
  int i = -1;
  for (auto it = bts_vec.begin(); it != bts_vec.end(); ++it)
  {
    tob_t tob{};
    tob.venue = v_names[++i];
    tob.bid_price = 6;
    tob.ask_price = 7;
    tob.bid_quantity = 21+to_type<aligned_t>(i);
    tob.ask_quantity = 19-to_type<aligned_t>(i);
    tob.symbol = symbol;
    it->get().push_back(tob);
  }
  // this should loop only once and find all tob's needed
  arb_trader.wait_till_ready();
  REQUIRE(arb_trader.all_markets_available());
  arb_trader.handle_update();
  REQUIRE(arb_trader.signal_buffer_size() == 0);
  arb_trader.signal();
  REQUIRE(arb_trader.signal_buffer_size() == 0);
  REQUIRE(arb_trader.execution_buffer_size() == 0);
  // update tob markets so there is an arbitrage opportunity
  {
    tob_t tob{};
    tob.bid_price = 6;
    tob.ask_price = 7;
    tob.bid_quantity = 15;
    tob.ask_quantity = 11;
    tob.symbol = symbol;
    tob.venue = venue1;
    bk_to_strat_buffer1.push_back(tob);
  }
  {
    tob_t tob{};
    tob.bid_price = 10;
    tob.ask_price = 12;
    tob.bid_quantity = 9;
    tob.ask_quantity = 7;
    tob.symbol = symbol;
    tob.venue = venue2;
    bk_to_strat_buffer2.push_back(tob);
  }
  // should funnel all updates into signal buffer
  arb_trader.handle_update();
  REQUIRE(arb_trader.signal_buffer_size() == 2);

  // after signal is called the second time, there should be two orders in the
  // execution buffer
  arb_trader.signal();
  arb_trader.signal();
  REQUIRE(arb_trader.execution_buffer_size() == 2);
  // check contents of execution buffer
  arbitrage_trader::execution_buffer& exec_buff =
    arb_trader.get_execution_buffer_reference();
  to_gateway_out_t to_gateway_out = exec_buff.pop_front();
  REQUIRE(to_gateway_out.side == side_type::bid_side);
  REQUIRE(to_gateway_out.symbol == symbol);
  REQUIRE(to_gateway_out.venue == venue1);
  REQUIRE(to_gateway_out.price == 7);
  REQUIRE(to_gateway_out.quantity == 9);

  to_gateway_out = exec_buff.pop_front();
  REQUIRE(to_gateway_out.side == side_type::ask_side);
  REQUIRE(to_gateway_out.symbol == symbol);
  REQUIRE(to_gateway_out.venue == venue2);
  REQUIRE(to_gateway_out.price == 10);
  REQUIRE(to_gateway_out.quantity == 9);
}

// TODO : test handle response (from order manager) of arbitrage trader
TEST_CASE("test handle response : arbitrage trader", "[handle_response_arb]")
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
  aligned_t id = 31;
  aligned_t b_px = 5;
  aligned_t b_qty = 11;
  from_gateway_out_t from_gateway_out{};
  from_gateway_out.price = b_px;
  from_gateway_out.quantity = b_qty;
  from_gateway_out.internal_order_id = id;
  // test accepted
  bid b = arb_trader.generate_bid(order_type::limit, symbol, venue, b_px, b_qty,
                                  id, arb_trader.get_time());
  arb_trader.add_quote(b);
  REQUIRE(arb_trader.total_quantity_quoting() == b_qty);
  from_gateway_out.status = status_type::accepted;
  om_to_strat_buffer_.push_back(from_gateway_out);
  arb_trader.handle_response();
  REQUIRE(arb_trader.total_quantity_quoting() == b_qty);

  // test rejected
  from_gateway_out.status = status_type::rejected;
  om_to_strat_buffer_.push_back(from_gateway_out);
  arb_trader.handle_response();
  REQUIRE(arb_trader.total_quantity_quoting() == 0);

  // test filled
  arb_trader.add_quote(b);
  from_gateway_out.status = status_type::filled;
  from_gateway_out.quantity = 5;
  om_to_strat_buffer_.push_back(from_gateway_out);
  arb_trader.handle_response();
  REQUIRE(arb_trader.total_quantity_quoting() == b_qty - 5);

}




#pragma clang diagnostic pop
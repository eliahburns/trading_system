//
// Created by eli on 8/19/17.
//

#include "../trading_system_viewer.hpp"
#include "catch.hpp"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "ClangTidyInspection"

TEST_CASE("test viewer symbol name look up from unsigned",
          "[viewer_sym_lookup]")
{
  using namespace aligned;

  static gw_to_om_buffer gw_to_om_b;
  static om_to_gw_buffer om_to_gw_b;
  static om_to_strat_buffer om_to_strat_b;
  static strat_to_om_buffer strat_to_om_b;
  order_manager::om_type test_id = 1;
  order_manager::om_type throttle = 2000000000; // 2 seconds
  order_manager order_man(test_id, throttle, om_to_gw_b,
                          gw_to_om_b, om_to_strat_b,
                          strat_to_om_b);

  REQUIRE(order_man.is_ready());

  static bk_to_strat_buffer bk_to_strat_buffer1;
  static om_to_strat_buffer om_to_strat_buffer_;
  static strat_to_om_buffer strat_to_om_buffer_;

  aligned_t arb_trdr_id = 5;

  arbitrage_trader arb_trader(arb_trdr_id,
                              om_to_strat_buffer_,
                              strat_to_om_buffer_);

  arb_trader.register_book_buffer(bk_to_strat_buffer1);
  REQUIRE(arb_trader.is_ready());

  trading_system_viewer viewer(order_man, arb_trader);

  auto symbol = viewer.translate_symbol_name(1);
  REQUIRE(symbol == symbol_name::symbol_one);
  symbol = viewer.translate_symbol_name(2);
  REQUIRE(symbol == symbol_name::symbol_two);
  symbol = viewer.translate_symbol_name(3);
  REQUIRE(symbol == symbol_name::symbol_three);
  symbol = viewer.translate_symbol_name(4);
  REQUIRE(symbol == symbol_name::symbol_four);
}

TEST_CASE("test viewer all books ready", "[viewer_all_books_reay]")
{
  static gw_to_om_buffer gw_to_om_b;
  static om_to_gw_buffer om_to_gw_b;
  static om_to_strat_buffer om_to_strat_b;
  static strat_to_om_buffer strat_to_om_b;
  order_manager::om_type test_id = 1;
  order_manager::om_type throttle = 2000000000; // 2 seconds
  order_manager order_man(test_id, throttle, om_to_gw_b,
                          gw_to_om_b, om_to_strat_b,
                          strat_to_om_b);

  REQUIRE(order_man.is_ready());

  static bk_to_strat_buffer bk_to_strat_buffer1;
  static om_to_strat_buffer om_to_strat_buffer_;
  static strat_to_om_buffer strat_to_om_buffer_;

  aligned_t arb_trdr_id = 5;

  arbitrage_trader arb_trader(arb_trdr_id,
                              om_to_strat_buffer_,
                              strat_to_om_buffer_);

  arb_trader.register_book_buffer(bk_to_strat_buffer1);
  REQUIRE(arb_trader.is_ready());

  trading_system_viewer viewer(order_man, arb_trader);

  // dummy circular buffers needed to construct book_builder class
  static gw_to_bk_buffer book_buffer;
  static bk_to_strat_buffer strat_buffer;
  const aligned_t book_id = 64;
  book_builder Book(book_id, book_buffer, strat_buffer);
  REQUIRE(Book.is_ready());

  viewer.register_book_builder(Book);
  REQUIRE(viewer.all_books_ready());

  const aligned_t book_id1 = 64;
  book_builder Book1(book_id1, book_buffer, strat_buffer);
  REQUIRE(Book1.is_ready());

  viewer.register_book_builder(Book1);
  REQUIRE(viewer.all_books_ready());

}


#pragma clang diagnostic pop
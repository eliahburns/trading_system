//
// Created by eli on 8/20/17.
//

#pragma clang diagnostic push
#pragma ide diagnostic ignored "ClangTidyInspection"
#define CATCH_CONFIG_MAIN

#include "catch.hpp"
#include "ipc_messages.hpp"
#include "buffer_types.hpp"
#include "book_builder.hpp"
#include "fake_gateway.hpp"


TEST_CASE("test book_builder with simulation data", "[bb_sim_data]")
{
  using namespace aligned;
  // dummy circular buffers needed to construct book_builder class
  auto sym = symbol_name::symbol_three;
  auto ven = venue_name::venue_four;
  static gw_to_bk_buffer book_buffer;
  static bk_to_strat_buffer strat_buffer;
  const aligned_t book_id = 64;
  book_builder Book(book_id, book_buffer, strat_buffer, sym, ven);
  REQUIRE(Book.is_ready());
  Book.drop_tob_updates();

  unsigned long long throttle_in1 = 10;

  std::string data_file_name{"../data/simulation_data.csv"};
  fake_gateway_in
    fake_in1(sym, ven, data_file_name, throttle_in1, book_buffer);

    std::cout << fake_in1.message_count() << std::endl;

  std::size_t count = 0;
  while (++count <= fake_in1.message_count())
  {
    auto msg = fake_in1.next_update();

    Book.update_book(msg);
    tob_t tob = Book.top_of_book();

    std::cout << "px: "<<tob.bid_price << "/" << tob.ask_price << std::endl;
    std::cout << "qty:"<<tob.bid_quantity << "/" << tob.ask_quantity << std::endl;

  }


}

TEST_CASE("test book_builder with simulation data handle_update",
          "[bb_sim_data]")
{
  using namespace aligned;
  // dummy circular buffers needed to construct book_builder class
  auto sym = symbol_name::symbol_three;
  auto ven = venue_name::venue_four;
  static gw_to_bk_buffer book_buffer;
  static bk_to_strat_buffer strat_buffer;
  const aligned_t book_id = 64;
  book_builder Book(book_id, book_buffer, strat_buffer, sym, ven);
  REQUIRE(Book.is_ready());
  Book.drop_tob_updates();

  unsigned long long throttle_in1 = 10;

  std::string data_file_name{"../data/simulation_data.csv"};
  fake_gateway_in
    fake_in1(sym, ven, data_file_name, throttle_in1, book_buffer);

  std::size_t count = 0;
  while (++count <= fake_in1.message_count())
  {
    fake_in1.release_next_update();
    Book.handle_update();
    tob_t tob = Book.top_of_book();
    REQUIRE(tob.symbol == sym);
    REQUIRE(tob.venue == ven);
  }

}


TEST_CASE("test book_builder with simulation data handle_update and fake "
            "gateway's main loop",
          "[bb_sim_data_fake_gw_main_loop]")
{
  using namespace aligned;
  // dummy circular buffers needed to construct book_builder class
  auto sym = symbol_name::symbol_three;
  auto ven = venue_name::venue_four;
  static gw_to_bk_buffer book_buffer;
  static bk_to_strat_buffer strat_buffer;
  const aligned_t book_id = 64;
  book_builder Book(book_id, book_buffer, strat_buffer, sym, ven);
  REQUIRE(Book.is_ready());
  Book.drop_tob_updates();

  unsigned long long throttle_in1 = 10;

  std::string data_file_name{"../data/simulation_data.csv"};
  fake_gateway_in
    fake_in1(sym, ven, data_file_name, throttle_in1, book_buffer);

  fake_in1.turn_on();
  fake_in1.in_main_loop();

  std::size_t count = 0;
  while (++count <= fake_in1.message_count())
  {
    Book.handle_update();
    tob_t tob = Book.top_of_book();
    REQUIRE(tob.symbol == sym);
    REQUIRE(tob.venue == ven);
  }
}

#pragma clang diagnostic pop


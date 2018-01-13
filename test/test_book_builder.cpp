//
// Created by eli on 8/13/17.
//


#include "../book_builder.hpp"
#include "catch.hpp"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "ClangTidyInspection"


TEST_CASE("test book_builder member: update_book", "[book_builder_update_bk]")
{
  using namespace aligned;
  // dummy circular buffers needed to construct book_builder class
  static gw_to_bk_buffer book_buffer;
  static bk_to_strat_buffer strat_buffer;
  const aligned_t book_id = 64;
  book_builder Book(book_id, book_buffer, strat_buffer);
  REQUIRE(Book.is_ready());
}

TEST_CASE("test book_builder update_book with message_t", "[bb_add_quote]")
{
  using namespace aligned;
  // dummy circular buffers needed to construct book_builder class
  static gw_to_bk_buffer book_buffer;
  static bk_to_strat_buffer strat_buffer;
  const aligned_t book_id = 64;
  book_builder Book(book_id, book_buffer, strat_buffer);
  REQUIRE(Book.is_ready());

  aligned_t bid_px_base = 15;
  aligned_t ask_px_base = 16;
  aligned_t limit = 10;
  // add bids
  for (aligned_t i = 0; i < limit; ++i)
  {
    message_t msg{};
    // minimum fields required for book update on add
    msg.type = order_type::add;
    msg.order_id = i;
    msg.side = side_type::bid_side;
    msg.price = bid_px_base - i;
    Book.update_book(msg);
    REQUIRE(Book.bid_count() == i+1);
  }
  // add asks
  for (aligned_t i = 0; i < limit; ++i)
  {
    message_t msg{};
    // minimum fields required for book update on add
    msg.type = order_type::add;
    msg.order_id = i + limit;
    msg.side = side_type::ask_side;
    msg.price = ask_px_base + i;
    Book.update_book(msg);
    REQUIRE(Book.ask_count() == i+1);
  }

  REQUIRE(Book.bid_count() == limit);
  REQUIRE(Book.ask_count() == limit);
}

TEST_CASE("test book_builder deletion of quote", "[bb_delete_quote]")
{
  using namespace aligned;
  // dummy circular buffers needed to construct book_builder class
  static gw_to_bk_buffer book_buffer;
  static bk_to_strat_buffer strat_buffer;
  const aligned_t book_id = 64;
  book_builder Book(book_id, book_buffer, strat_buffer);
  REQUIRE(Book.is_ready());

  aligned_t bid_px_base = 15;
  aligned_t ask_px_base = 16;
  aligned_t limit = 10;
  // add bids
  for (aligned_t i = 0; i < limit; ++i)
  {
    message_t msg{};
    // minimum fields required for book update on add
    msg.type = order_type::add;
    msg.order_id = i;
    msg.side = side_type::bid_side;
    msg.price = bid_px_base - i;
    Book.update_book(msg);
    REQUIRE(Book.bid_count() == i+1);
  }
  // add asks
  for (aligned_t i = 0; i < limit; ++i)
  {
    message_t msg{};
    // minimum fields required for book update on add
    msg.type = order_type::add;
    msg.order_id = i + limit;
    msg.side = side_type::ask_side;
    msg.price = ask_px_base + i;
    Book.update_book(msg);
    REQUIRE(Book.ask_count() == i+1);
  }

  // now delete all quotes that were just added (w/o having price in message_t)
  for (aligned_t i = 0; i < limit; ++i)
  {
    message_t msg{};
    // minimum fields required for book update on add
    msg.type = order_type::delete_;
    msg.order_id = i;
    msg.side = side_type::bid_side;
    Book.update_book(msg);
    REQUIRE(Book.bid_count() == limit - (i+1));
  }
  for (aligned_t i = 0; i < limit; ++i)
  {
    message_t msg{};
    // minimum fields required for book update on add
    msg.type = order_type::delete_;
    msg.order_id = i + limit;
    msg.side = side_type::ask_side;
    Book.update_book(msg);
    REQUIRE(Book.ask_count() == limit - (i+1));
  }

  REQUIRE(Book.bid_count() == 0);
  REQUIRE(Book.ask_count() == 0);
}

TEST_CASE("test book_builder update_book with modify", "[bb_modify_quote]")
{
  using namespace aligned;
  // dummy circular buffers needed to construct book_builder class
  static gw_to_bk_buffer book_buffer;
  static bk_to_strat_buffer strat_buffer;
  const aligned_t book_id = 64;
  book_builder Book(book_id, book_buffer, strat_buffer);
  REQUIRE(Book.is_ready());

  aligned_t bid_px_base = 15;
  aligned_t ask_px_base = 16;
  aligned_t limit = 3;
  aligned_t start_qty = 25;
  // add bids
  for (aligned_t i = 0; i < limit; ++i)
  {
    message_t msg{};
    // minimum fields required for book update on add
    msg.type = order_type::add;
    msg.order_id = i;
    msg.side = side_type::bid_side;
    msg.price = bid_px_base - i;
    msg.quantity = start_qty;
    Book.update_book(msg);
    REQUIRE(Book.bid_count() == i+1);
  }
  // modify bids (reduce quantity)
  for (aligned_t i = 0; i < limit; ++i)
  {
    message_t msg{};
    // minimum fields required for book update on add
    msg.type = order_type::modify;
    msg.order_id = i;
    msg.side = side_type::bid_side;
    msg.quantity = start_qty - i;
    Book.update_book(msg);
    REQUIRE(Book.bid_count() == limit);
  }
}


TEST_CASE("test book_builder delete after modify (decrease)",
          "[bb_modify_then_delete_quote]")
{
  using namespace aligned;
  // dummy circular buffers needed to construct book_builder class
  static gw_to_bk_buffer book_buffer;
  static bk_to_strat_buffer strat_buffer;
  const aligned_t book_id = 64;
  book_builder Book(book_id, book_buffer, strat_buffer);
  REQUIRE(Book.is_ready());

  aligned_t bid_px_base = 15;
  aligned_t ask_px_base = 16;
  aligned_t limit = 3;
  aligned_t start_qty = 25;
  // add bids
  for (aligned_t i = 0; i < limit; ++i)
  {
    message_t msg{};
    // minimum fields required for book update on add
    msg.type = order_type::add;
    msg.order_id = i;
    msg.side = side_type::bid_side;
    msg.price = bid_px_base - i;
    msg.quantity = start_qty;
    Book.update_book(msg);
    REQUIRE(Book.bid_count() == i+1);
  }
  // modify bids (reduce quantity)
  for (aligned_t i = 0; i < limit; ++i)
  {
    message_t msg{};
    // minimum fields required for book update on add
    msg.type = order_type::modify;
    msg.order_id = i;
    msg.side = side_type::bid_side;
    msg.quantity = start_qty - i;
    Book.update_book(msg);
    REQUIRE(Book.bid_count() == limit);
  }
  for (aligned_t i = 0; i < limit; ++i)
  {
    message_t msg{};
    // minimum fields required for book update on add
    msg.type = order_type::delete_;
    msg.order_id = i;
    msg.side = side_type::bid_side;
    Book.update_book(msg);
    REQUIRE(Book.bid_count() == limit - (i+1));
  }

  REQUIRE(Book.bid_count() == 0);
}

TEST_CASE("test delete after modify with increase",
          "[delete_modify_increase_qty]")
{
  using namespace aligned;
  // dummy circular buffers needed to construct book_builder class
  static gw_to_bk_buffer book_buffer;
  static bk_to_strat_buffer strat_buffer;
  const aligned_t book_id = 64;
  book_builder Book(book_id, book_buffer, strat_buffer);
  REQUIRE(Book.is_ready());

  aligned_t bid_px_base = 15;
  aligned_t ask_px_base = 16;
  aligned_t limit = 3;
  aligned_t start_qty = 25;
  // add bids
  for (aligned_t i = 0; i < limit; ++i)
  {
    message_t msg{};
    // minimum fields required for book update on add
    msg.type = order_type::add;
    msg.order_id = i;
    msg.side = side_type::bid_side;
    msg.price = bid_px_base - i;
    msg.quantity = start_qty;
    Book.update_book(msg);
    REQUIRE(Book.bid_count() == i+1);
  }
  // modify bids (reduce quantity)
  for (aligned_t i = 0; i < limit; ++i)
  {
    message_t msg{};
    // minimum fields required for book update on add
    msg.type = order_type::modify;
    msg.order_id = i;
    msg.side = side_type::bid_side;
    msg.quantity = start_qty + i*10;
    Book.update_book(msg);
    REQUIRE(Book.bid_count() == limit);
  }
  for (aligned_t i = 0; i < limit; ++i)
  {
    message_t msg{};
    // minimum fields required for book update on add
    msg.type = order_type::delete_;
    msg.order_id = i;
    msg.side = side_type::bid_side;
    Book.update_book(msg);
    REQUIRE(Book.bid_count() == limit - (i+1));
  }

  REQUIRE(Book.bid_count() == 0);
}

TEST_CASE("test book builder top of book", "[top_of_book]")
{
  using namespace aligned;
  // dummy circular buffers needed to construct book_builder class
  static gw_to_bk_buffer book_buffer;
  static bk_to_strat_buffer strat_buffer;
  const aligned_t book_id = 64;
  book_builder Book(book_id, book_buffer, strat_buffer);
  REQUIRE(Book.is_ready());

  aligned_t bid_px_base = 15;
  aligned_t ask_px_base = 16;
  aligned_t limit = 10;
  aligned_t qty = 25;
  // add bids
  for (aligned_t i = 0; i < limit; ++i)
  {
    message_t msg{};
    // minimum fields required for book update on add
    msg.type = order_type::add;
    msg.order_id = i;
    msg.side = side_type::bid_side;
    msg.price = bid_px_base - i;
    msg.quantity = qty;
    Book.update_book(msg);
    REQUIRE(Book.bid_count() == i+1);
  }
  // add asks
  for (aligned_t i = 0; i < limit; ++i)
  {
    message_t msg{};
    // minimum fields required for book update on add
    msg.type = order_type::add;
    msg.order_id = i + limit;
    msg.side = side_type::ask_side;
    msg.price = ask_px_base + i;
    msg.quantity = qty;
    Book.update_book(msg);
    REQUIRE(Book.ask_count() == i+1);
  }
  REQUIRE(Book.bid_count() == limit);
  REQUIRE(Book.ask_count() == limit);
  {
    tob_t tob = Book.top_of_book();
    REQUIRE(tob.bid_price == bid_px_base);
    REQUIRE(tob.ask_price == ask_px_base);
    REQUIRE(tob.bid_quantity == qty);
    REQUIRE(tob.ask_quantity == qty);
  }
  // add an order at the best bid/ask and check top of book again
  {
    aligned_t id = 20;
    message_t msg{};
    msg.type = order_type::add;
    msg.order_id = id;
    msg.side = side_type::ask_side;
    msg.price = ask_px_base;
    msg.quantity = qty;
    Book.update_book(msg);
  }
  {
    aligned_t id = 21;
    message_t msg{};
    msg.type = order_type::add;
    msg.order_id = id;
    msg.side = side_type::bid_side;
    msg.price = bid_px_base;
    msg.quantity = qty;
    Book.update_book(msg);
  }

  {
    tob_t tob = Book.top_of_book();
    REQUIRE(tob.bid_price == bid_px_base);
    REQUIRE(tob.ask_price == ask_px_base);
    REQUIRE(tob.bid_quantity == 2*qty);
    REQUIRE(tob.ask_quantity == 2*qty);
  }
  // delete best bids and take top of book again
  {
    aligned_t id = 21;
    message_t msg{};
    msg.type = order_type::delete_;
    msg.order_id = id;
    msg.side = side_type::bid_side;
    Book.update_book(msg);
  }
  {
    aligned_t id = 0;
    message_t msg{};
    msg.type = order_type::delete_;
    msg.order_id = id;
    msg.side = side_type::bid_side;
    Book.update_book(msg);
  }
  {
    tob_t tob = Book.top_of_book();
    REQUIRE(tob.bid_price == --bid_px_base);
    REQUIRE(tob.ask_price == ask_px_base);
    REQUIRE(tob.bid_quantity == qty);
    REQUIRE(tob.ask_quantity == 2*qty);
  }
  // delete best asks and take top of book
  {
    aligned_t id = 20;
    message_t msg{};
    msg.type = order_type::delete_;
    msg.order_id = id;
    msg.side = side_type::ask_side;
    Book.update_book(msg);
  }
  {
    aligned_t id = 10;
    message_t msg{};
    msg.type = order_type::delete_;
    msg.order_id = id;
    msg.side = side_type::ask_side;
    Book.update_book(msg);
  }
  {
    tob_t tob = Book.top_of_book();
    REQUIRE(tob.bid_price == bid_px_base);
    REQUIRE(tob.ask_price == ++ask_px_base);
    REQUIRE(tob.bid_quantity == qty);
    REQUIRE(tob.ask_quantity == qty);
  }
  // widen the bid ask spread and confirm top of book reflects changes
  // extra note: deleting quote requires order_id, order_type (delete_) and side
  for (aligned_t i = 1; i < limit-1; ++i)
  {
    message_t del_bid{};
    message_t del_ask{};
    del_bid.order_id = i;
    del_ask.order_id = i + limit;
    del_bid.type = order_type::delete_;
    del_ask.type = order_type::delete_;
    del_bid.side = side_type::bid_side;
    del_ask.side = side_type::ask_side;
    Book.update_book(del_bid);
    Book.update_book(del_ask);

    tob_t tob = Book.top_of_book();
    REQUIRE(tob.bid_price == --bid_px_base);
    REQUIRE(tob.ask_price == ++ask_px_base);
    REQUIRE(tob.bid_quantity == qty);
    REQUIRE(tob.ask_quantity == qty);
  }

}

TEST_CASE("test repetitive add and delete affects on top of book",
          "[repeat_add_delete_tob]")
{
  using namespace aligned;
  // dummy circular buffers needed to construct book_builder class
  static gw_to_bk_buffer book_buffer;
  static bk_to_strat_buffer strat_buffer;
  const aligned_t book_id = 64;
  book_builder Book(book_id, book_buffer, strat_buffer);
  REQUIRE(Book.is_ready());

  aligned_t bid_px_base = 15;
  aligned_t ask_px_base = 16;
  aligned_t limit = 10;
  aligned_t qty = 25;
  // add bids
  for (aligned_t i = 0; i < limit; ++i)
  {
    message_t msg{};
    // minimum fields required for book update on add
    msg.type = order_type::add;
    msg.order_id = i;
    msg.side = side_type::bid_side;
    msg.price = bid_px_base - i;
    msg.quantity = qty;
    Book.update_book(msg);
    REQUIRE(Book.bid_count() == i+1);
  }
  // add asks
  for (aligned_t i = 0; i < limit; ++i)
  {
    message_t msg{};
    // minimum fields required for book update on add
    msg.type = order_type::add;
    msg.order_id = i + limit;
    msg.side = side_type::ask_side;
    msg.price = ask_px_base + i;
    msg.quantity = qty;
    Book.update_book(msg);
    REQUIRE(Book.ask_count() == i+1);
  }
  REQUIRE(Book.bid_count() == limit);
  REQUIRE(Book.ask_count() == limit);
  {
    tob_t tob = Book.top_of_book();
    REQUIRE(tob.bid_price == bid_px_base);
    REQUIRE(tob.ask_price == ask_px_base);
    REQUIRE(tob.bid_quantity == qty);
    REQUIRE(tob.ask_quantity == qty);
  }

  aligned_t add_del_qty = 31;
  for (aligned_t i = 0; i < limit; ++i)
  {
    if (i % 2 == 0) // delete tob
    {
      message_t del_bid{};
      message_t del_ask{};
      del_bid.order_id = 0;
      del_ask.order_id = 10;
      del_bid.type = order_type::delete_;
      del_ask.type = order_type::delete_;
      del_bid.side = side_type::bid_side;
      del_ask.side = side_type::ask_side;
      Book.update_book(del_bid);
      Book.update_book(del_ask);

      tob_t tob = Book.top_of_book();
      REQUIRE(tob.bid_price == --bid_px_base);
      REQUIRE(tob.ask_price == ++ask_px_base);
      REQUIRE(tob.bid_quantity == qty);
      REQUIRE(tob.ask_quantity == qty);

    }
    else // add to top of book
    {
      message_t add_bid{};
      message_t add_ask{};
      add_bid.order_id = 0;
      add_ask.order_id = 10;
      add_bid.price = ++bid_px_base;
      add_ask.price = --ask_px_base;
      add_bid.quantity = add_del_qty;
      add_ask.quantity = add_del_qty;
      add_bid.type = order_type::add;
      add_ask.type = order_type::add;
      add_bid.side = side_type::bid_side;
      add_ask.side = side_type::ask_side;
      Book.update_book(add_bid);
      Book.update_book(add_ask);

      tob_t tob = Book.top_of_book();
      REQUIRE(tob.bid_price == bid_px_base);
      REQUIRE(tob.ask_price == ask_px_base);
      REQUIRE(tob.bid_quantity == add_del_qty);
      REQUIRE(tob.ask_quantity == add_del_qty);
    }
  }
}




#pragma clang diagnostic pop
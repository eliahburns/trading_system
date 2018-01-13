//
// Created by eli on 8/12/17.
//

#include "catch.hpp"
#include "../book.hpp"


#pragma clang diagnostic push
#pragma ide diagnostic ignored "ClangTidyInspection"

aligned::message_t generate_message(std::uint64_t px, std::uint64_t id)
{
  aligned::message_t msg{};

  msg.price = px;
  msg.order_id = id;
  msg.quantity = 5;
  msg.type = aligned::order_type::limit;

  return msg;
}


TEST_CASE("book add ask quote", "[add_quote]")
{
  using uint64_t = std::uint64_t;
  book<ask> ask_book;
  const uint64_t px = 50;
  auto msg = generate_message(px, 1);

  // test adding one quote
  ask_book.add_quote(msg);
  ask find_msg = ask_book.find_quote(px, 1);
  REQUIRE(find_msg.price == px);
  REQUIRE(find_msg.order_id == 1);
  REQUIRE(ask_book.quote_count() == 1);

  // add more quotes
  for (aligned::aligned_t i = 2; i < 50; ++i)
  {
    auto msg1 = generate_message(px, i);
    ask_book.add_quote(msg1);
    ask find_msg1 = ask_book.find_quote(px, i);
    REQUIRE(find_msg1.price == px);
    REQUIRE(find_msg1.order_id == i);
    REQUIRE(ask_book.quote_count() == i);

  }
}

TEST_CASE("book delete ask quote",  "[delete_quote]")
{
  using uint64_t = std::uint64_t;
  book<ask> ask_book;
  const uint64_t px = 50;
  auto msg = generate_message(px, 1);

  // test adding one quote
  ask_book.add_quote(msg);
  ask find_msg = ask_book.find_quote(px, 1);
  REQUIRE(find_msg.price == px);
  REQUIRE(find_msg.order_id == 1);
  REQUIRE(ask_book.quote_count() == 1);

  ask_book.delete_quote(msg, px);
  REQUIRE(ask_book.quote_count() == 0);

  // add more quotes
  for (aligned::aligned_t i = 1; i < 50; ++i)
  {
    auto msg1 = generate_message(px, i);
    ask_book.add_quote(msg1);
    ask find_msg1 = ask_book.find_quote(px, i);
    REQUIRE(find_msg1.price == px);
    REQUIRE(find_msg1.order_id == i);
    REQUIRE(ask_book.quote_count() == i);
  }
  // delete quotes
  auto num_quotes = ask_book.quote_count();
  for (aligned::aligned_t i = 2; i < 50; ++i)
  {
    auto msg1 = generate_message(px, i);
    ask_book.delete_quote(msg1, px);
    REQUIRE(ask_book.quote_count() == --num_quotes);
  }
}


TEST_CASE("get best ask", "[best_ask]")
{
  using uint64_t = std::uint64_t;
  book<ask> ask_book;
  const uint64_t px = 50;

  for (uint64_t i = 0; i < 10; ++i)
  {
    auto msg = generate_message(px+i, i);
    ask_book.add_quote(msg);
  }

  auto best_px = ask_book.best_price();
  REQUIRE(best_px == px);
}


TEST_CASE("get best bid", "[best_bid]")
{
  using uint64_t = std::uint64_t;
  // bid book should use std::greater<> for comparison amongst keys
  book<bid, std::greater<aligned::aligned_t>> bid_book;
  const uint64_t px = 50;

  uint64_t limit = 10;
  for (uint64_t i = 0; i <= limit; ++i)
  {
    auto msg = generate_message(px+i, i);
    bid_book.add_quote(msg);
  }

  auto best_px = bid_book.best_price();
  REQUIRE(best_px == px+limit);
}


TEST_CASE("test the modify member", "[modify_bid]")
{
  using uint64_t = std::uint64_t;
  // bid book should use std::greater<> for comparison amongst keys
  book<bid, std::greater<aligned::aligned_t>> bid_book;
  const uint64_t px = 50;

  uint64_t limit = 10;
  for (uint64_t i = 1; i <= limit; ++i)
  {
    auto msg = generate_message(px, i);
    bid_book.add_quote(msg);
  }
  auto best_px = bid_book.best_price();
  REQUIRE(best_px == px);

  // increase size of first order and assert it's pushed to the back of price
  auto msg = generate_message(px, 1);
  msg.quantity += 5;
  bid_book.modify_quote(msg, px);
  auto& p_list = bid_book.quotes_at_price_level(px);

  REQUIRE(p_list.back().order_id == 1);
}


TEST_CASE("test modify or delete", "[modify_delete]")
{
  using uint64_t = std::uint64_t;
  // bid book should use std::greater<> for comparison amongst keys
  book<bid, std::greater<aligned::aligned_t>> bid_book;
  const uint64_t px = 50;
  auto msg = generate_message(px, 1);
  bid_book.add_quote(msg);

  // increase size of first order and assert it's pushed to the back of price
  auto msg_modify = generate_message(px, 1);
  msg_modify.quantity += 5;
  msg_modify.type = aligned::order_type::modify;
  bid_book.modify_or_delete_quote(msg_modify, px);
  auto& p_list = bid_book.quotes_at_price_level(px);

  REQUIRE(p_list.back().order_id == 1);

  auto msg_delete = generate_message(px, 1);
  msg_delete.type = aligned::order_type::delete_;
  bid_book.modify_or_delete_quote(msg_delete, px);
  p_list = bid_book.quotes_at_price_level(px);

  REQUIRE(p_list.size() == 0);
}

TEST_CASE("test the ask quote volume between two prices",
          "[ask_volume_between]")
{
  using uint64_t = std::uint64_t;
  // bid book should use std::greater<> for comparison amongst keys
  book<ask, std::less<aligned::aligned_t>> ask_book;
  const uint64_t px = 50;
  const uint64_t qty = 7;

  uint64_t limit = 10;
  for (uint64_t i = 0; i < limit; ++i)
  {
    auto id = i;
    auto msg = generate_message(px + i, id);
    msg.quantity = qty;
    auto msg2 = generate_message(px+i, id+limit);
    msg2.quantity = qty;
    ask_book.add_quote(msg);
    ask_book.add_quote(msg2);
  }
  REQUIRE(ask_book.quote_count() == 2*limit);

  auto cum_vol = ask_book.volume_between_inclusive<ask>(px, px+limit);
  REQUIRE(cum_vol == qty*limit*2);
}

TEST_CASE("test ask the quote volume between two prices when prices have only "
            "one quote per level", "[volume_between_one_quote_per_level]")
{
  using uint64_t = std::uint64_t;
  // ask book should use std::less<> for comparison amongst keys,
  // which is the default template parameter.
  book<ask, std::less<aligned::aligned_t>> ask_book;
  const uint64_t px = 50;
  const uint64_t qty = 7;

  uint64_t limit = 10;
  for (uint64_t i = 0; i < limit; ++i)
  {
    auto id = i;
    auto msg = generate_message(px + i, id);
    msg.quantity = qty;
    ask_book.add_quote(msg);
  }

  REQUIRE(ask_book.quote_count() == limit);

  auto cum_vol = ask_book.volume_between_inclusive<ask>(px, px+limit);
  REQUIRE(cum_vol == qty*limit);
}


TEST_CASE("test bid quote volume between two price levels", "[bid_vol_between]")
{
  using uint64_t = std::uint64_t;
  // ask book should use std::less<> for comparison amongst keys
  // this is actually a default template parameter
  const uint64_t px = 50;
  const uint64_t qty = 7;

  uint64_t limit = 10;
  // bid book should use std::greater<> for comparison amongst keys
  book<bid, std::greater<aligned::aligned_t>> bid_book;

  for (uint64_t i = 0; i < limit; ++i)
  {
    uint64_t id = i;
    auto msg = generate_message(px + i, id);
    msg.quantity = qty;
    bid_book.add_quote(msg);
  }

  //auto cum_vol = bid_book.volume_between_inclusive<bid>(px, px+limit);
  //REQUIRE(cum_vol == qty*limit);
}

TEST_CASE("test if bid and ask are different types", "[bid_ask_is_same]")
{
  const bool quote_type_check = std::is_same<bid, ask>::value;
  const bool quote_type_check2 = std::is_same<ask, bid>::value;

  REQUIRE_FALSE(quote_type_check);
  REQUIRE_FALSE(quote_type_check2);
}

TEST_CASE("test list delete : used often in book", "[list_delete_quote]")
{
  using namespace std;
  using namespace aligned;
  list<bid> bids{};
  aligned_t limit = 10;
  aligned_t px = 13;
  aligned_t qty = 7;
  for (aligned_t i = 0; i < limit; ++i)
  {
    message_t msg{};
    msg.price = px;
    msg.order_id = i;
    msg.quantity = qty;
    bid b(msg);
    bids.push_back(b);
    REQUIRE(bids.size() == i+1);
  }
  for (aligned_t i = 0; i < limit; ++i)
  {
    message_t msg{};
    msg.price = px;
    msg.order_id = i;
    msg.quantity = qty;
    bid b(msg);
    bids.remove(b);
    REQUIRE(bids.size() == limit-(i+1));
  }
  REQUIRE(bids.size() == 0);
}



#pragma clang diagnostic pop
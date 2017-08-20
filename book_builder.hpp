//
// Created by eli on 8/11/17.
//

#ifndef TRADING_SYSTEM_COMPONENTS_BOOK_BUILDER_HPP
#define TRADING_SYSTEM_COMPONENTS_BOOK_BUILDER_HPP

#include <list>
#include <unordered_map>
#include <type_traits>
#include "trading_system_component.hpp"
#include "quote.hpp"
#include "book.hpp"


class book_builder : public trading_system_component<book_builder>
{
public:
  //using trading_system_component<book_builder>::trading_system_component;

  using bids = book<bid, std::greater<aligned::aligned_t>>;
  using asks = book<ask>;

  using id_price_map = // to look up
    std::unordered_map<aligned::aligned_t, aligned::aligned_t>;


  book_builder(
    tsc_type book_builder_id,
    aligned::gw_to_bk_buffer& gw_to_bk_buffer1,
    aligned::bk_to_strat_buffer& bk_to_strat_buffer1
  );

  void component_main_loop() override;

  void handle_update() override;

  void handle_response() override;

  const bool is_ready() override;

  void log_update() override;

  void update_book(aligned::message_t msg);

  aligned::tob_t top_of_book();

  const aligned::aligned_t ask_count() const { return asks_.quote_count(); }
  const aligned::aligned_t bid_count() const { return bids_.quote_count(); }

  void print_asks() { std::cout << "asks:\n" << asks_ << std::endl; }
  void print_bids() { std::cout << "bids:\n" << bids_ << std::endl; }

  void drop_tob_updates();

private:
  const bool tob_is_same(aligned::tob_t& new_tob) const;

  aligned::gw_to_bk_buffer& gw_to_bk_buffer_;
  aligned::bk_to_strat_buffer& bk_to_strat_buffer_;
  id_price_map id_price_map_;
  bids bids_;
  asks asks_;
  aligned::tob_t last_tob_change_;
  std::string collection_name_;
  aligned::tob_buffer_t tob_buffer_;
};


#endif //TRADING_SYSTEM_COMPONENTS_BOOK_BUILDER_HPP

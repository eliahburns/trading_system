//
// Created by eli on 8/8/17.
//

#ifndef TRADING_SYSTEM_COMPONENTS_TRADING_SYSTEM_VIEWER_HPP
#define TRADING_SYSTEM_COMPONENTS_TRADING_SYSTEM_VIEWER_HPP

#include "order_manager.hpp"
#include "arbitrage_trader.hpp"
#include "book_builder.hpp"


class trading_system_viewer
{
public:
  using books_vec_t =
  std::vector<std::reference_wrapper<book_builder>>;

  // takes as arguments references to all components in trading system
  trading_system_viewer(order_manager& order_manager1,
    arbitrage_trader& arbitrage_trader1);

  trading_system_viewer(const trading_system_viewer&) = delete;
  trading_system_viewer&operator=(const trading_system_viewer&) = delete;
  trading_system_viewer&operator=(const trading_system_viewer&&) = delete;
  trading_system_viewer(const trading_system_viewer&&) = delete;

  ~trading_system_viewer() = default;

  void viewer_main_loop();

  void register_book_builder(
    std::reference_wrapper<book_builder> book_builder_ref);

  void turn_off_trading();

  void wait_till_ready();

  aligned::symbol_name translate_symbol_name(unsigned symbol);

  void current_pnl();

  void start_trading();

  void stop_trading();

  void adjust_stop_loss();

  void cancel_all_orders();

  void drop_all_logs();

  const bool all_books_ready();

private:
  order_manager& order_manager_;
  arbitrage_trader& arbitrage_trader_;
  books_vec_t books_vec_;
};


#endif //TRADING_SYSTEM_COMPONENTS_TRADING_SYSTEM_VIEWER_HPP

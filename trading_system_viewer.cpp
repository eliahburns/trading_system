//
// Created by eli on 8/8/17.
//

#include "trading_system_viewer.hpp"

trading_system_viewer::trading_system_viewer(
  order_manager &order_manager1, arbitrage_trader &arbitrage_trader1,
  fake_gateway_out& fake_gateway_out1)
  : order_manager_(order_manager1), arbitrage_trader_(arbitrage_trader1),
    fake_gateway_out_(fake_gateway_out1)
{
  arbitrage_trader_.cease_new_positions();  // prevent trading before user ready
}

void trading_system_viewer::viewer_main_loop()
{
  wait_till_ready();
  // get user input
  std::string input_;
  while(on_)
  {
    // break if user wants to quit
    if (input_ == "start_trading")
      start_trading();
    else if (input_ == "stop_trading")
      stop_trading();
    else if (input_ == "adjust_stop_loss")
      adjust_stop_loss();
    else if (input_ == "current_pnl")
      current_pnl();
    else if (input_ == "cancel_all_orders")
      cancel_all_orders();
    else if (input_ == "turn_on_gateways")
      turn_on_gateways();
    std::cin >> input_;
    if (input_ == "quit")
      on_ = false;
  }
  stop_trading();
  //while (arbitrage_trader_.has_position()) ;
  turn_off_trading();
  turn_off_order_manager();
  turn_off_books();
  turn_off_gateways();
}

aligned::symbol_name
trading_system_viewer::translate_symbol_name(unsigned symbol)
{
  using aligned::symbol_name;
  symbol_name symbol_n;
  symbol_n = (symbol == 1) ? symbol_name::symbol_one : symbol_name::symbol_two;
  if (symbol == 3)
    symbol_n = symbol_name::symbol_three;
  if (symbol == 4)
    symbol_n = symbol_name::symbol_four;

  return symbol_n;
}

void trading_system_viewer::current_pnl()
{
  std::cout << "Enter symbol to get PnL (1, 2, 3, or 4): "
            << std::endl;
  unsigned sym = 0;
  std::cin >> sym;
  auto symbol = translate_symbol_name(sym);
  std::cout << "current pnl for arbitrage trader is: " <<
            arbitrage_trader_.current_pnl(symbol) << std::endl;
}

void trading_system_viewer::adjust_stop_loss()
{
  std::cout << "Enter new stop loss value (default=4): " << std::endl;
  double new_stop = 4;
  std::cin >> new_stop;
  arbitrage_trader_.set_stop_loss(new_stop);
  std::cout << "...new stop loss value set to " << new_stop << std::endl;
}

void trading_system_viewer::start_trading()
{
  arbitrage_trader_.start_new_positions();
  std::cout << "...ready to trade" << std::endl;
}

void trading_system_viewer::stop_trading()
{
  arbitrage_trader_.cease_new_positions();
  std::cout << "...stopping all trading" << std::endl;
}

void trading_system_viewer::cancel_all_orders()
{
  // TODO : implement with strategy or arbitrage_trader
  std::cout << "...canceling all orders" << std::endl;
}

void trading_system_viewer::turn_off_order_manager()
{
  std::cout << "...turning off order manager" << std::endl;
  order_manager_.shut_down();
}

void trading_system_viewer::wait_till_ready()
{
  std::string input_;

  while (!(arbitrage_trader_.is_ready() && order_manager_.is_ready() &&
           all_books_ready()) && input_ != "abort")
  {
    std::cout << "waiting for all trading system components ready."
              << std::endl;
    std::cout << "enter 'abort' to cancel start up." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cin >> input_;
  }
  // start with clean logs for testing and demo
  drop_all_logs();

  if (input_ == "abort")
    on_ = false;
}

void trading_system_viewer::register_book_builder(
  std::reference_wrapper<book_builder> book_builder_ref)
{
  books_vec_.push_back(book_builder_ref);
}

const bool trading_system_viewer::all_books_ready()
{
  for (book_builder& bk : books_vec_)
    if (!bk.is_ready())
      return false;
  return true;
}

void trading_system_viewer::drop_all_logs()
{
  for (book_builder& bk : books_vec_)
    bk.drop_tob_updates();

  arbitrage_trader_.drop_all_orders();
  order_manager_.drop_all_log_messages();
  arbitrage_trader_.drop_pnl_logs();
}

void trading_system_viewer::turn_on_gateways()
{
  std::cout << "...turning on gateways" << std::endl;
  for (fake_gateway_in& gw : gateways_in_vec_)
    gw.turn_on();
  // fake gateway out is on by default
}

void trading_system_viewer::turn_off_gateways()
{
  for (fake_gateway_in& gw : gateways_in_vec_)
    gw.turn_off();
  fake_gateway_out_.turn_off();
}

void trading_system_viewer::turn_off_trading()
{
  arbitrage_trader_.turn_off();
}

void trading_system_viewer::turn_off_books()
{
  for (book_builder& bb : books_vec_)
    bb.turn_off();
}


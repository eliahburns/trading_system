//
// Created by eli on 8/14/17.
//

#ifndef TRADING_SYSTEM_COMPONENTS_ARBITRAGE_TRADER_HPP
#define TRADING_SYSTEM_COMPONENTS_ARBITRAGE_TRADER_HPP

#include "strategy.hpp"


class arbitrage_trader : public strategy<arbitrage_trader>
{
public:
  using strategy<arbitrage_trader>::strategy;

  static const std::uint64_t buff_size = 256;

  using signal_buffer =
  aligned_circular_buffer<aligned::tob_t, buff_size>;

  using execution_buffer =
  aligned_circular_buffer<aligned::to_gateway_out_t, buff_size>;

  using quotes_sent =
  std::multimap<aligned::symbol_name, to_gateway_out_t>;

  void signal() override;

  void execution() override;

  void handle_update() override;

  void handle_response() override;

  void component_main_loop() override;

  void wait_till_ready() override;

  bool all_markets_available();

  const aligned::aligned_t
  signal_buffer_size() { return signal_buffer_.size(); }

  const aligned::aligned_t
  execution_buffer_size() { return execution_buffer_.size(); }

  signal_buffer& get_signal_buffer_reference() { return signal_buffer_; }

  execution_buffer&
  get_execution_buffer_reference() { return execution_buffer_; }

  void to_signal_buffer(const tob_t& tob) { signal_buffer_.push_back(tob); }

  void to_execution_buffer(const to_gateway_out_t& to_gateway_out)
  { execution_buffer_.push_back(to_gateway_out); }

  tob_t signal_buffer_front() { return  signal_buffer_.pop_front(); }

  to_gateway_out_t execution_buffer_front()
  { return execution_buffer_.pop_front(); }

  void add_quote_sent(const to_gateway_out_t& to_gateway_out);

  void cleanup_quote_sent(from_gateway_out_t from_gateway_out);

  const bool quote_just_sent(to_gateway_out_t to_gateway_out);

  void set_stop_loss(double stop_loss) { stop_loss_ = stop_loss; }

  void check_stop_loss(symbol_name symbol);

  aligned::tob_t best_market_to_exit_in(symbol_name symbol, side_type side);

  void cease_new_positions() { open_new_trades_ = false; }

  void start_new_positions() { open_new_trades_ = true; }

  void turn_off() {implementation::component_ready_ = false; }

private:
  signal_buffer signal_buffer_;
  execution_buffer execution_buffer_;
  quotes_sent quotes_sent_;
  double stop_loss_{4};
  bool open_new_trades_{true};
};



#endif //TRADING_SYSTEM_COMPONENTS_ARBITRAGE_TRADER_HPP


#include <cstdlib>

#include "buffer_types.hpp"
#include "arbitrage_trader.hpp"
#include "book_builder.hpp"
#include "order_manager.hpp"
#include "fake_gateway.hpp"
#include "trading_system_viewer.hpp"

class barrier
{
public:
  explicit barrier(std::size_t count)
    : count_{count}
  { }

  void wait()
  {
    std::unique_lock<std::mutex> lock{mutex_};
    if (--count_ == 0) {
      cv_.notify_all();
    } else {
      cv_.wait(lock, [this] { return count_ == 0; });
    }
  }
private:
  std::mutex mutex_;
  std::condition_variable cv_;
  std::size_t count_;
};





using namespace aligned;

barrier barrier1(4);

int main()
{
  // example using a symbol listed at two different exchanges with fake gateways
  static gw_to_om_buffer gw_to_om_b;
  static gw_to_bk_buffer gw_to_bk_b_sym1_ven1;
  static gw_to_bk_buffer gw_to_bk_b_sym1_ven2;
  static om_to_gw_buffer om_to_gw_b;
  static om_to_strat_buffer om_to_strat_b;
  static strat_to_om_buffer strat_to_om_b;
  static bk_to_strat_buffer bk_to_strat_b_sym1_ven1;
  static bk_to_strat_buffer bk_to_strat_b_sym1_ven2;

  unsigned long long throttle_in1 = 1511;
  unsigned long long throttle_in2 = 811;
  unsigned long long throttle_out = 2000000000; // 2 seconds
  unsigned long long om_throttle = 100000000;
  auto symbol1 = symbol_name::symbol_one;
  auto venue1 = venue_name::venue_one;
  auto venue2 = venue_name::venue_two;
  std::string data_file_name{"../data/simulation_data.csv"};
  aligned_t bb_id1 = 3;
  aligned_t bb_id2 = 5;
  aligned_t trader_id = 7;
  aligned_t om_id = 9;

  // components and gateways
  fake_gateway_in
    fake_in1(symbol1, venue1, data_file_name, throttle_in1,
             gw_to_bk_b_sym1_ven1);
  fake_gateway_in
    fake_in2(symbol1, venue2, data_file_name, throttle_in2,
             gw_to_bk_b_sym1_ven2);
  book_builder
    book_sym1_ven1(bb_id1, gw_to_bk_b_sym1_ven1, bk_to_strat_b_sym1_ven1,
      symbol1, venue1);
  book_builder
    book_sym1_ven2(bb_id2, gw_to_bk_b_sym1_ven2, bk_to_strat_b_sym1_ven2,
      symbol1, venue2);
  order_manager
    order_man(om_id, om_throttle, om_to_gw_b, gw_to_om_b, om_to_strat_b,
              strat_to_om_b);
  fake_gateway_out fake_out(throttle_out, gw_to_om_b, om_to_gw_b);
  arbitrage_trader arb_trader(trader_id, om_to_strat_b, strat_to_om_b);
  trading_system_viewer viewer(order_man, arb_trader, fake_out);

  viewer.register_book_builder(book_sym1_ven1);
  viewer.register_book_builder(book_sym1_ven2);
  viewer.register_gateway_in(fake_in1);
  viewer.register_gateway_in(fake_in2);

  arb_trader.register_market(symbol1, venue1);
  arb_trader.register_market(symbol1, venue2);
  arb_trader.register_book_buffer(bk_to_strat_b_sym1_ven1);
  arb_trader.register_book_buffer(bk_to_strat_b_sym1_ven2);

  unsigned px_offset = 0;
  fake_in1.set_price_offset(px_offset);

  std::thread v_thread;
  std::thread om_thread;
  std::thread f_out_thread;
  std::thread book_thread1;
  std::thread book_thread2;
  std::thread arb_thread;
  std::thread fake_in1_thread;
  std::thread fake_in2_thread;

  v_thread = std::thread(&trading_system_viewer::viewer_main_loop, &viewer);
  om_thread = std::thread(&order_manager::component_main_loop, &order_man);
  f_out_thread = std::thread(&fake_gateway_out::out_main_loop, &fake_out);
  book_thread1 = std::thread(&book_builder::component_main_loop,&book_sym1_ven1);
  book_thread2 = std::thread(&book_builder::component_main_loop,&book_sym1_ven2);
  arb_thread = std::thread(&arbitrage_trader::component_main_loop, &arb_trader);
  fake_in1_thread = std::thread(&fake_gateway_in::in_main_loop, &fake_in1);
  fake_in2_thread = std::thread(&fake_gateway_in::in_main_loop, &fake_in2);

  //barrier1.wait();

  viewer.start_trading();
  viewer.turn_on_gateways();



  fake_in1_thread.join();
  fake_in2_thread.join();
  arb_thread.join();
  book_thread2.join();
  book_thread1.join();
  f_out_thread.join();
  om_thread.join();
  v_thread.join();

  return EXIT_SUCCESS;
}


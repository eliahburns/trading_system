#include <cstdlib>
#include "buffer_types.hpp"
#include "arbitrage_trader.hpp"
#include "book_builder.hpp"
#include "order_manager.hpp"
#include "fake_gateway.hpp"
#include "trading_system_viewer.hpp"


int main()
{
  // example using a symbol listed at two different exchanges with fake gateways
  using namespace aligned;
  static gw_to_om_buffer gw_to_om_b;
  static gw_to_bk_buffer gw_to_bk_b_sym1_ven1;
  static gw_to_bk_buffer gw_to_bk_b_sym1_ven2;
  static om_to_gw_buffer om_to_gw_b;
  static om_to_strat_buffer om_to_strat_b;
  static strat_to_om_buffer strat_to_om_b;
  static bk_to_strat_buffer bk_to_strat_b_sym1_ven1;
  static bk_to_strat_buffer bk_to_strat_b_sym1_ven2;

  unsigned throttle_in1 = 1000000000; // 1 second
  unsigned throttle_in2 = 1500000000; // 1.5 seconds
  unsigned throttle_out = 2000000000; // 2 seconds
  unsigned om_throttle = 100000000;
  auto symbol1 = symbol_name::symbol_one;
  auto venue1 = venue_name::venue_one;
  auto venue2 = venue_name::venue_two;
  std::string data_file_name{"../data/simulation_data.csv"};
  aligned_t bb_id1 = 000;
  aligned_t bb_id2 = 001;
  aligned_t trader_id = 010;
  aligned_t om_id = 011;

  // components and gateways
  fake_gateway_in
    fake_in1(symbol1, venue1, data_file_name, throttle_in1,
             gw_to_bk_b_sym1_ven1);
  fake_gateway_in
    fake_in2(symbol1, venue2, data_file_name, throttle_in2,
             gw_to_bk_b_sym1_ven2);
  book_builder
    book_sym1_ven1(bb_id1, gw_to_bk_b_sym1_ven1, bk_to_strat_b_sym1_ven1);
  book_builder
    book_sym1_ven2(bb_id2, gw_to_bk_b_sym1_ven2, bk_to_strat_b_sym1_ven2);
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

  std::thread v_thread;
  v_thread = std::thread(&trading_system_viewer::viewer_main_loop, &viewer);
  std::thread om_thread;
  om_thread = std::thread(&order_manager::component_main_loop, &order_man);
  std::thread f_in_thread1;
  f_in_thread1 = std::thread(&fake_gateway_in::in_main_loop, &fake_in1);
  std::thread f_in_thread2;
  f_in_thread2 = std::thread(&fake_gateway_in::in_main_loop, &fake_in2);
  std::thread f_out_thread;
  f_out_thread = std::thread(&fake_gateway_out::out_main_loop, &fake_out);
  std::thread book_thread1;
  book_thread1 = std::thread(&book_builder::component_main_loop,
                             &book_sym1_ven1);
  std::thread book_thread2;
  book_thread2 = std::thread(&book_builder::component_main_loop,
                             &book_sym1_ven2);
  std::thread arb_thread;
  arb_thread = std::thread(&arbitrage_trader::component_main_loop, &arb_trader);

  arb_thread.join();
  book_thread2.join();
  book_thread1.join();
  f_out_thread.join();
  f_in_thread2.join();
  f_in_thread1.join();
  om_thread.join();
  v_thread.join();

  return EXIT_SUCCESS;
}


/*
int main(int argc, char* argv[])
{

  //
  static const std::uint64_t k_buff_size = 512;
  static aligned_circular_buffer<aligned::to_gateway_out_t, k_buff_size>
    send_gw_out_buffer;
  static aligned_circular_buffer<aligned::from_gateway_out_t, k_buff_size>
    recv_gw_out_buffer;
  static aligned_circular_buffer<aligned::to_gateway_out_t, k_buff_size>
    recv_strat_buffer;
  static aligned_circular_buffer<aligned::from_gateway_out_t, k_buff_size>
    send_strat_buffer;

  order_manager::om_type test_id = 1;
  order_manager::om_type throttle = 2000000000; // 2 seconds

  order_manager orderManager(test_id, throttle, send_gw_out_buffer,
                             recv_gw_out_buffer, send_strat_buffer,
                             recv_strat_buffer);

  trading_system_viewer ts_viewer(orderManager);

  std::cout << "order_manager id = " << orderManager.id() << std::endl;
  orderManager.is_ready();

  std::thread order_manager_thread;
  std::thread viewer_thread;

  order_manager_thread =
    std::thread(&order_manager::component_main_loop, &orderManager);

  viewer_thread
    = std::thread(&trading_system_viewer::viewer_main_loop, &ts_viewer);

  viewer_thread.join();
  order_manager_thread.join();

  // check message that's up in send_gw_out_buffer
  auto msg = send_gw_out_buffer.pop_front();
  std::cout << "msg.id = " << msg.internal_order_id << std::endl;

  // confirm enum works as expected
  auto t = aligned::order_type::limit;
  if (t == aligned::order_type::limit)
    std::cout << "t == limit order type" << std::endl;

  return EXIT_SUCCESS;
}

*/

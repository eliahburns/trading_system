#include <cstdlib>
#include "buffer_types.hpp"
#include "arbitrage_trader.hpp"
#include "book_builder.hpp"
#include "order_manager.hpp"
#include "fake_gateway.hpp"

int main()
{
  // example using a symbol listed at two different exchanges with fake gateways
  using namespace aligned;
  static gw_to_om_buffer gw_to_om_b;
  static gw_to_bk_buffer gw_to_bk_b_sym1_ven1;
  static gw_to_bk_buffer gw_to_bk_b_sym1_ven2;
  static om_to_gw_buffer om_to_gw_b;
  static om_to_strat_buffer om_to_strat_b;
  static bk_to_strat_buffer bk_to_strat_b_sym1_ven1;
  static bk_to_strat_buffer bk_to_strat_b_sym1_ven2;

  unsigned throttle_in1 = 100000000;
  unsigned throttle_in2 = 150000000;
  unsigned throttle_out = 200000000;

  auto symbol1 = symbol_name::symbol_one;
  auto venue1 = venue_name::venue_one;
  auto venue2 = venue_name::venue_two;
  std::string data_file_name{"../data/simulation_data.csv"};
  aligned_t bb_id1 = 000;
  aligned_t bb_id2 = 001;
  aligned_t trader_id = 010;
  aligned_t om_id = 011;

  fake_gateway_in fake_in1(symbol1, venue1, data_file_name, throttle_in1,
                           gw_to_bk_b_sym1_ven1);

  fake_gateway_in fake_in2(symbol1, venue2, data_file_name, throttle_in2,
                           gw_to_bk_b_sym1_ven2);

  book_builder book_sym1_ven1(bb_id1, gw_to_bk_b_sym1_ven1,
                              bk_to_strat_b_sym1_ven1);

  book_builder book_sym1_ven2(bb_id2, gw_to_bk_b_sym1_ven2,
                              bk_to_strat_b_sym1_ven2);

  return EXIT_SUCCESS;
}


/*
void write_text_to_log_file( const std::string &text )
{
  std::ofstream log_file("log_file.txt", std::ios::out | std::ios::app );
  log_file << text;
  log_file << std::endl;
}


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

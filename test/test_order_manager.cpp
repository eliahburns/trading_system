//
// Created by eli on 8/13/17.
//

#include "../order_manager.hpp"
#include "catch.hpp"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "ClangTidyInspection"

TEST_CASE("test order_manager constructor", "[order_manager_constr]")
{
  using namespace aligned;

  static gw_to_om_buffer gwToOmBuffer;
  static om_to_gw_buffer omToGwBuffer;
  static om_to_strat_buffer omToStratBuffer;
  static strat_to_om_buffer stratToOmBuffer;
  order_manager::om_type test_id = 1;
  order_manager::om_type throttle = 2000000000; // 2 seconds
  order_manager orderManager(test_id, throttle, omToGwBuffer,
                             gwToOmBuffer, omToStratBuffer,
                             stratToOmBuffer);
  REQUIRE(orderManager.is_ready());
}

TEST_CASE("test order manager personal id member", "[om_personal_id]")
{
  using namespace aligned;

  static gw_to_om_buffer gw_to_om_b;
  static om_to_gw_buffer om_to_gw_b;
  static om_to_strat_buffer om_to_strat_b;
  static strat_to_om_buffer strat_to_om_b;
  order_manager::om_type test_id = 1;
  order_manager::om_type throttle = 2000000000; // 2 seconds
  order_manager order_man(test_id, throttle, om_to_gw_b,
                             gw_to_om_b, om_to_strat_b,
                             strat_to_om_b);
  REQUIRE(order_man.is_ready());

  aligned_t strat_id = 5;
  aligned_t strat_id1 = 6;
  aligned_t strat_id2 = 13;

  auto om_id = order_man.personal_id(strat_id);
  auto om_id1 = order_man.personal_id(strat_id1);

  aligned_t om_ids = 0;
  REQUIRE(om_id == ++om_ids);
  REQUIRE(om_id1 == ++om_ids);
  auto om_id2 = order_man.personal_id(strat_id);
  REQUIRE(om_id2 == 1); // identical to the first

  auto s_id_back = order_man.get_strategy_order_id(om_ids);
  REQUIRE(s_id_back == strat_id1);
  auto s_id_back1 = order_man.get_strategy_order_id(om_ids - 1);
  REQUIRE(s_id_back1 == strat_id);

  auto om_id3 = order_man.personal_id(strat_id2);
  REQUIRE(om_id3 == ++om_ids);
  auto s_id_back2 = order_man.get_strategy_order_id(om_ids);
  REQUIRE(s_id_back2 == strat_id2);
}

TEST_CASE("test handle update and release order to gateway", "[update_gateway]")
{
  using namespace aligned;

  static gw_to_om_buffer gw_to_om_b;
  static om_to_gw_buffer om_to_gw_b;
  static om_to_strat_buffer om_to_strat_b;
  static strat_to_om_buffer strat_to_om_b;
  order_manager::om_type test_id = 1;
  order_manager::om_type throttle = 2000000000; // 2 seconds
  order_manager order_man(test_id, throttle, om_to_gw_b,
                          gw_to_om_b, om_to_strat_b,
                          strat_to_om_b);

  aligned_t px = 11;
  aligned_t qty = 19;
  aligned_t id = 13;
  auto side = side_type::bid_side;
  auto sym = symbol_name::symbol_three;
  auto ven = venue_name::venue_one;
  auto o_type = order_type::ioc;

  to_gateway_out_t to_gw_out{};
  to_gw_out.internal_order_id = id;
  to_gw_out.price = px;
  to_gw_out.quantity = qty;
  to_gw_out.side = side;
  to_gw_out.symbol = sym;
  to_gw_out.venue = ven;
  to_gw_out.type = o_type;
  to_gw_out.event_time = order_man.get_time();

  REQUIRE(order_man.is_ready());
  REQUIRE(om_to_gw_b.empty());

  strat_to_om_b.push_back(to_gw_out);

  order_man.handle_update();
  REQUIRE(om_to_gw_b.empty());

  order_man.release_order_to_gateway();
  REQUIRE_FALSE(om_to_gw_b.empty());
  om_to_gw_b.pop_front();
  REQUIRE(om_to_gw_b.empty());
  to_gw_out.internal_order_id += 1;
  strat_to_om_b.push_back(to_gw_out);

  order_man.handle_update();
  REQUIRE(om_to_gw_b.empty());

  // after sending the first order, we should have to wait 2 seconds until we
  // send the next order--according to the throttle set above in the constructor
  std::this_thread::sleep_for(std::chrono::seconds(1));
  order_man.release_order_to_gateway();
  REQUIRE(om_to_gw_b.empty());

  std::this_thread::sleep_for(std::chrono::seconds(1));
  order_man.release_order_to_gateway();
  REQUIRE_FALSE(om_to_gw_b.empty());
}

TEST_CASE("test order manager handle response", "[om_handle_response]")
{
  using namespace aligned;

  static gw_to_om_buffer gw_to_om_b;
  static om_to_gw_buffer om_to_gw_b;
  static om_to_strat_buffer om_to_strat_b;
  static strat_to_om_buffer strat_to_om_b;
  order_manager::om_type test_id = 1;
  order_manager::om_type throttle = 2000000000; // 2 seconds
  order_manager order_man(test_id, throttle, om_to_gw_b,
                          gw_to_om_b, om_to_strat_b,
                          strat_to_om_b);

  aligned_t px = 11;
  aligned_t qty = 19;
  aligned_t id = 13;
  auto side = side_type::bid_side;
  auto sym = symbol_name::symbol_three;
  auto ven = venue_name::venue_one;
  auto status = status_type::filled;
  auto o_type = order_type::ioc;

  to_gateway_out_t to_gw_out{};
  to_gw_out.internal_order_id = id;
  to_gw_out.price = px;
  to_gw_out.quantity = qty;
  to_gw_out.side = side;
  to_gw_out.symbol = sym;
  to_gw_out.venue = ven;
  to_gw_out.type = o_type;
  to_gw_out.event_time = order_man.get_time();

  REQUIRE(order_man.is_ready());
  REQUIRE(om_to_gw_b.empty());

  strat_to_om_b.push_back(to_gw_out);

  order_man.handle_update();
  REQUIRE(om_to_gw_b.empty());

  order_man.release_order_to_gateway();
  REQUIRE_FALSE(om_to_gw_b.empty());

  aligned_t order_manager_id = 1;
  from_gateway_out_t from_gw_out{};
  from_gw_out.internal_order_id = order_manager_id;
  from_gw_out.symbol = sym;
  from_gw_out.venue = ven;
  from_gw_out.side = side;
  from_gw_out.price = px;
  from_gw_out.quantity = qty;
  from_gw_out.status = status;
  from_gw_out.event_time = order_man.get_time();

  REQUIRE(gw_to_om_b.empty());
  gw_to_om_b.push_back(from_gw_out);
  REQUIRE_FALSE(gw_to_om_b.empty());

  REQUIRE(om_to_strat_b.empty());
  order_man.handle_response();
  REQUIRE(gw_to_om_b.empty());
  REQUIRE_FALSE(om_to_strat_b.empty());

  order_man.drop_all_log_messages();
}

TEST_CASE("test logging in order manager", "[om_logging]")
{
  using namespace aligned;

  static gw_to_om_buffer gw_to_om_b;
  static om_to_gw_buffer om_to_gw_b;
  static om_to_strat_buffer om_to_strat_b;
  static strat_to_om_buffer strat_to_om_b;
  order_manager::om_type test_id = 1;
  order_manager::om_type throttle = 2000000000; // 2 seconds
  order_manager order_man(test_id, throttle, om_to_gw_b,
                          gw_to_om_b, om_to_strat_b,
                          strat_to_om_b);

  REQUIRE(order_man.is_ready());

  REQUIRE(order_man.log_message_count() == 0);

  aligned_t limit = 37;
  for (aligned_t i = 0; i < limit; ++i)
  {
    REQUIRE(order_man.log_message_count() == i);
    order_man.log_update();
  }
  order_man.drop_all_log_messages();
  REQUIRE(order_man.log_message_count() == 0);
}

#pragma clang diagnostic pop
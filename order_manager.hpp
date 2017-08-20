//
// Created by eli on 8/8/17.
//

#ifndef TRADING_SYSTEM_COMPONENTS_ORDER_MANAGER_HPP
#define TRADING_SYSTEM_COMPONENTS_ORDER_MANAGER_HPP

#include <iostream>
#include <queue>
#include <unordered_map>
#include "trading_system_component.hpp"


class order_manager : public trading_system_component<order_manager>
{
public:
  using om_type = std::uint64_t;

  using order_queue = std::queue<aligned::to_gateway_out_t>;
  using order_id_map = std::unordered_map<om_type, om_type>;


  order_manager(
    om_type id, om_type order_send_throttle,
    aligned::om_to_gw_buffer& send_gw_buffer1,
    aligned::gw_to_om_buffer& recv_gw_buffer1,
    aligned::om_to_strat_buffer& send_strat_buffer1,
    aligned::strat_to_om_buffer& recv_strat_buffer1
  )
    : trading_system_component(id),
      order_send_throttle_(order_send_throttle),
      om_to_gw_buffer_(send_gw_buffer1),
      gw_to_om_buffer_(recv_gw_buffer1),
      om_to_strat_buffer_(send_strat_buffer1),
      strat_to_om_buffer_(recv_strat_buffer1), internal_order_id_(0),
      last_send_time_(0)
  {
    collection_name_ = "order_manager" + std::to_string(component_id_);
    component_ready_ = true;
  }

  void handle_update() override;

  void handle_response() override;

  const bool is_ready() override;

  void log_update() override;

  void log_order_send(aligned::to_gateway_out_t& to_gateway_out);

  void
  log_order_exchange_response(aligned::from_gateway_out_t& from_gateway_out);

  void log_adjust_open_orders(aligned::from_gateway_out_t& from_gateway_out);

  void log_alert_message_queue_alert();

  void component_main_loop() override;

  void cancel_orders_in_queue();

  void release_order_to_gateway();

  const om_type generate_new_order_id() { return ++internal_order_id_; }

  om_type personal_id(aligned::aligned_t strategy_order_id);

  om_type get_strategy_order_id(aligned::aligned_t order_manager_order_id)
  { return to_strategy_id_map_.find(order_manager_order_id)->second; }

  void drop_all_log_messages();

  aligned::aligned_t log_message_count();

private:
  om_type order_send_throttle_;
  aligned::om_to_gw_buffer& om_to_gw_buffer_;
  aligned::gw_to_om_buffer& gw_to_om_buffer_;
  aligned::om_to_strat_buffer& om_to_strat_buffer_;
  aligned::strat_to_om_buffer& strat_to_om_buffer_;

  om_type internal_order_id_;
  om_type last_send_time_;

  order_queue outgoing_order_queue_;

  // order id maps:
  order_id_map  to_strategy_id_map_; // key: order manager, val: strategy
  order_id_map  from_strategy_id_map_; // key: strategy, val: order manager
  std::string collection_name_;
};


#endif //TRADING_SYSTEM_COMPONENTS_ORDER_MANAGER_HPP

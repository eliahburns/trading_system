//
// Created by eli on 8/10/17.
//

#include "order_manager.hpp"
//#include <cassert>


void order_manager::handle_update()
{
  // check if there's new data in receive buffer
  if (!strat_to_om_buffer_.empty())
  {
    aligned::to_gateway_out_t to_gw_out = strat_to_om_buffer_.pop_front();
    // generate new order id that's used with order manager
    to_gw_out.internal_order_id = personal_id(to_gw_out.internal_order_id);
    to_gw_out.event_time = get_time();

    outgoing_order_queue_.push(to_gw_out);
  }
}

void order_manager::handle_response()
// Check if there is a message in the buffer from gateway out.
// If there is, check if message says order was filled, canceled etc
// and respond accordingly, log details
{
  if (!gw_to_om_buffer_.empty())
  {
    aligned::from_gateway_out_t msg = gw_to_om_buffer_.pop_front();
    // swap order id's and forward to strategy
    msg.internal_order_id = get_strategy_order_id(msg.internal_order_id);
    om_to_strat_buffer_.push_back(msg);
    log_order_exchange_response(msg);
  }
 }

const bool order_manager::is_ready()
{
  return component_ready_;
}


void order_manager::component_main_loop()
{
  while (!shut_down_ && component_ready_)
  {
    handle_update();

    handle_response();

    release_order_to_gateway();
  }
    component_ready_ = false;
    cancel_orders_in_queue();
}

void order_manager::cancel_orders_in_queue()
// release all orders from queue--flesh out later
{
  while (!outgoing_order_queue_.empty())
    outgoing_order_queue_.pop();
}

void order_manager::release_order_to_gateway()
// this function should have its own thread, eventually, that runs while
// system is ready--implemented in order managers main loop
{
  if (!outgoing_order_queue_.empty())
    // if throttle time met
    if ((get_time()-last_send_time_) >= order_send_throttle_)
    {
      const unsigned k_queue_warning_size = 100;
      if (outgoing_order_queue_.size() > k_queue_warning_size)
        log_alert_message_queue_alert();

      auto msg = outgoing_order_queue_.front();
      outgoing_order_queue_.pop();
      // send message to gateway out and update last send time
      om_to_gw_buffer_.push_back(msg);
      last_send_time_ = get_time();

      // log that order has been sent to gateway out
      log_order_send(msg);
    }
}

void order_manager::log_order_send(aligned::to_gateway_out_t &to_gateway_out)
{
  auto collection = conn_[collection_name_]["log_messages"];
  using namespace aligned;
  bsoncxx::builder::stream::document builder =
    bsoncxx::builder::stream::document{};
  bsoncxx::document::value doc_value = builder
    << "type" << "order_send"
    << "time" << std::to_string(to_gateway_out.event_time)
    << "info" << bsoncxx::builder::stream::open_document
    << "internal_order_id" << std::to_string(to_gateway_out.internal_order_id)
    << "event_time" << std::to_string(to_gateway_out.event_time)
    << "symbol" << enum_str(to_gateway_out.symbol)
    << "venue" << enum_str(to_gateway_out.venue)
    << "price" << std::to_string(to_gateway_out.price)
    << "quantity" << std::to_string(to_gateway_out.quantity)
    << "side" << enum_str(to_gateway_out.side)
    << "type" << enum_str(to_gateway_out.type)
    << bsoncxx::builder::stream::close_document
    << bsoncxx::builder::stream::finalize;

  bsoncxx::document::view view = doc_value.view();
  collection.insert_one(view);
}


void order_manager::log_order_exchange_response(
  aligned::from_gateway_out_t &from_gateway_out)
{
  auto collection = conn_[collection_name_]["log_messages"];
  using namespace aligned;

  bsoncxx::builder::stream::document builder =
    bsoncxx::builder::stream::document{};
  bsoncxx::document::value doc_value = builder
    << "type" << "order_response"
    << "time" << std::to_string(from_gateway_out.event_time)
    << "info" << bsoncxx::builder::stream::open_document
    << "internal_order_id" << std::to_string(from_gateway_out.internal_order_id)
    << "event_time" << std::to_string(from_gateway_out.event_time)
    << "symbol" << enum_str(from_gateway_out.symbol)
    << "venue" << enum_str(from_gateway_out.venue)
    << "price" << std::to_string(from_gateway_out.price)
    << "quantity" << std::to_string(from_gateway_out.quantity)
    << "side" << enum_str(from_gateway_out.side)
    << "status" << enum_str(from_gateway_out.status)
    << bsoncxx::builder::stream::close_document
    << bsoncxx::builder::stream::finalize;

  bsoncxx::document::view view = doc_value.view();
  collection.insert_one(view);
}


void order_manager::log_alert_message_queue_alert()
{
  auto collection = conn_[collection_name_]["log_messages"];

  auto queue_size = outgoing_order_queue_.size();
  std::string warning_msg = "Warning: Outgoing order queue has size: "
                            + std::to_string(queue_size);
  bsoncxx::builder::stream::document builder =
    bsoncxx::builder::stream::document{};
  bsoncxx::document::value doc_value = builder
    << "type" << "alert"
    << "time" << std::to_string(get_time())
    << "info" << bsoncxx::builder::stream::open_document
    << "message" << warning_msg
    << bsoncxx::builder::stream::close_document
    << bsoncxx::builder::stream::finalize;

  bsoncxx::document::view view = doc_value.view();
  collection.insert_one(view);
}

order_manager::om_type
order_manager::personal_id(aligned::aligned_t strategy_order_id)
{
  // see if id already exists
  auto it = from_strategy_id_map_.find(strategy_order_id);
  if (it == from_strategy_id_map_.end())
  {
    auto new_id = generate_new_order_id();
    to_strategy_id_map_.emplace(new_id, strategy_order_id);
    from_strategy_id_map_.emplace(strategy_order_id, new_id);
    return new_id;
  }
  return it->second;
}

void order_manager::drop_all_log_messages()
{
  std::string messages{"log_messages"};
  auto collection = conn_[collection_name_][messages];
  collection.drop();
}

aligned::aligned_t order_manager::log_message_count()
{
  std::string messages{"log_messages"};
  auto collection = conn_[collection_name_][messages];
  mongocxx::cursor cursor =  collection.find(document{} << finalize);

  aligned::aligned_t doc_count = 0;
  for (auto doc : cursor)
    ++doc_count;
  return doc_count;
}

void order_manager::log_update()
{
  std::string messages{"log_messages"};
  auto collection = conn_[collection_name_][messages];
  std::string test_msg = "test : add log message to database";
  bsoncxx::builder::stream::document builder =
    bsoncxx::builder::stream::document{};
  bsoncxx::document::value doc_value = builder
    << "type" << "test_log_message"
    << "time" << std::to_string(get_time())
    << "info" << bsoncxx::builder::stream::open_document
    << "message" << test_msg
    << bsoncxx::builder::stream::close_document
    << bsoncxx::builder::stream::finalize;

  bsoncxx::document::view view = doc_value.view();
  collection.insert_one(view);
}



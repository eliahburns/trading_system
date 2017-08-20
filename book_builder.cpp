//
// Created by eli on 8/11/17.
//

#include "book_builder.hpp"

book_builder::book_builder(trading_system_component::tsc_type book_builder_id,
                           aligned::gw_to_bk_buffer &gw_to_bk_buffer1,
                           aligned::bk_to_strat_buffer &bk_to_strat_buffer1)
  : trading_system_component(book_builder_id),
    gw_to_bk_buffer_(gw_to_bk_buffer1),
    bk_to_strat_buffer_(bk_to_strat_buffer1)
{
  collection_name_ += "book_builder_" + std::to_string(book_builder_id);
  component_ready_ = true;
}

void book_builder::update_book(aligned::message_t msg)
// check if msg is delete or modify type and then look up in
// id_price_map, else add order to bids or asks and store in id_price_map
{
  if (msg.type == aligned::order_type::add)
  {
    // add new quote to proper book side
    // add key, val pair to id_price_map
    if (msg.side == aligned::side_type::ask_side)
      asks_.add_quote(msg);
    else
      bids_.add_quote(msg);
    id_price_map_.emplace(msg.order_id, msg.price);
  }
  else if (msg.type == aligned::order_type::modify)
  {
    // look up price corresponding to order id, to find order quicker in book
    auto px = id_price_map_.find(msg.order_id)->second;
    if (msg.side == aligned::side_type::ask_side)
      asks_.modify_quote(msg, px);
    else
      bids_.modify_quote(msg, px);

  }
  else if (msg.type == aligned::order_type::delete_)
  {
    // look up price corresponding to order id
    // remove quote to proper book side
    // remove key, val pair from id_price_map
    auto px = id_price_map_.find(msg.order_id)->second;
    if (msg.side == aligned::side_type::ask_side)
      asks_.delete_quote(msg, px);
    else
      bids_.delete_quote(msg, px);
    id_price_map_.erase(msg.order_id);
  }
  else
  {
    std::cout << "Warning: unexpected message type in update_book" << std::endl;
  }
}

void book_builder::handle_update()
// Check if buffer from gateway in is not empty. If message, then update book
// according to the type of message we've received from exchange: add, delete..
{
  if (!gw_to_bk_buffer_.empty())
  {
    auto msg = gw_to_bk_buffer_.pop_front();
    update_book(msg);

    // calculate top of book after updating book
    auto tob = top_of_book();
    if (!tob_is_same(tob))
    {
      bk_to_strat_buffer_.push_back(tob);
      tob_buffer_.push_back(tob);
      last_tob_change_ = tob;
      log_update();
    }
  }
}

void book_builder::handle_response()
// book_builder is currently only forwarding top of book updates to strategy,
// with no response from strategy.
{ ; }

const bool book_builder::is_ready()
{
  return component_ready_;
}

void book_builder::component_main_loop()
{
  while (component_ready_)
  {
    handle_update();

    handle_response();
  }
}


aligned::tob_t book_builder::top_of_book()
{
  aligned::tob_t tob{};

  tob.ask_price = asks_.best_price();
  tob.bid_price = bids_.best_price();

  tob.ask_quantity =
    asks_.volume_between_inclusive<ask>(tob.ask_price, tob.ask_price);
  tob.bid_quantity =
    bids_.volume_between_inclusive<bid>(tob.bid_price, tob.bid_price);

  tob.book_timestamp = get_time();

  return tob;
}

const bool book_builder::tob_is_same(aligned::tob_t &new_tob) const
{
  if (new_tob.bid_price == 0 || new_tob.ask_price == 0)
    return true;
  if (last_tob_change_.bid_price != new_tob.bid_price)
    return false;
  if (last_tob_change_.ask_price != new_tob.ask_price)
    return false;
  if (last_tob_change_.bid_quantity != new_tob.bid_quantity)
    return false;
  return last_tob_change_.ask_quantity == new_tob.ask_quantity;
}

void book_builder::log_update()
// we want to log updates for each message received / tob message sent.
// TODO: decide on how many collections to use for logging
{
  using namespace aligned;
  if (!tob_buffer_.empty())
  {
    aligned::tob_t tob = tob_buffer_.pop_front();
    std::string tob_updates{"tob_updates"};
    auto collection = conn_[collection_name_][tob_updates];
    bsoncxx::builder::stream::document builder =
      bsoncxx::builder::stream::document{};
    bsoncxx::document::value doc_value = builder
      << "symbol" << enum_str(tob.symbol)
      << "time" << std::to_string(get_time())
      << "info" << bsoncxx::builder::stream::open_document
      << "best_bid_price" << std::to_string(tob.bid_price)
      << "best_ask_price" << std::to_string(tob.ask_price)
      << "best_bid_quantity" << std::to_string(tob.bid_quantity)
      << "best_ask_quantity" << std::to_string(tob.ask_quantity)
      << "venue" << enum_str(tob.venue)
      << "book_timestamp" << std::to_string(tob.book_timestamp)
      << bsoncxx::builder::stream::close_document
      << bsoncxx::builder::stream::finalize;

    bsoncxx::document::view view = doc_value.view();
    collection.insert_one(view);
  }
}

void book_builder::drop_tob_updates()
{
  std::string tob_updates{"tob_updates"};
  auto collection = conn_[collection_name_][tob_updates];
  collection.drop();
}



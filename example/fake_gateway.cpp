//
// Created by eli on 8/19/17.
//

#include "fake_gateway.hpp"

#include <tuple>
#include <thread>

std::istream& operator>>(std::istream& str, csv_row& data)
{
  data.read_next_row(str);
  return str;
}


fake_gateway_in::fake_gateway_in(aligned::symbol_name symbol,
                                 aligned::venue_name venue,
                                 std::string data_file_name,
                                 unsigned long throttle_milli,
                                 aligned::gw_to_bk_buffer& to_bk_buffer)
  : symbol_(symbol), venue_(venue), data_file_name_(data_file_name),
    throttle_(throttle_milli), to_bk_buffer_(to_bk_buffer)
{
  // set up message vector
  using namespace aligned;

  std::ifstream file(data_file_name_);
  if (!file.is_open())
    throw std::runtime_error{"file is not open"};
  csv_row row;
  file >> row;
  // add first two messages for market
  message_t b_msg{};
  message_t a_msg{};
  message_t prev_b_msg{};
  message_t prev_a_msg{};
  std::tie(b_msg, a_msg) = make_message(row);
  b_msg.type = order_type::add;
  a_msg.type = order_type::add;
  b_msg.order_id = new_id();
  a_msg.order_id = new_id();
  a_msg.symbol = symbol_;
  b_msg.symbol = symbol_;
  a_msg.venue = venue_;
  b_msg.venue = venue_;
  // time gets add when message is put into book buffer
  messages_.push_back(b_msg);
  messages_.push_back(a_msg);
  prev_a_msg = a_msg;
  prev_b_msg = b_msg;

  while(file >> row)
  {
    std::tie(b_msg, a_msg) = make_message(row);
    add_modify_delete_message(b_msg, prev_b_msg);
    add_modify_delete_message(a_msg, prev_a_msg);
    prev_a_msg = a_msg;
    prev_b_msg = b_msg;
  }
  file.close();
}

std::pair<aligned::message_t, aligned::message_t>
fake_gateway_in::make_message(csv_row row)
{
  aligned::message_t msg{};
  msg.symbol = symbol_;
  msg.venue = venue_;
  msg.price = row[0];
  msg.quantity = row[2];
  msg.side = aligned::side_type::bid_side;

  aligned::message_t msg1{};
  msg1.symbol = symbol_;
  msg1.venue = venue_;
  msg1.price = row[1];
  msg1.quantity = row[3];
  msg1.side = aligned::side_type::ask_side;

  return std::make_pair(msg, msg1);
}

void fake_gateway_in::add_modify_delete_message(aligned::message_t& msg,
                                                aligned::message_t& prev_msg)
{
  if (msg.price != prev_msg.price)
  {
    // delete previous message and add new with new order id
    prev_msg.type = aligned::order_type::delete_;
    aligned::message_t new_msg{};
    new_msg = prev_msg;
    messages_.emplace_back(new_msg);
    msg.order_id = new_id();
    msg.type = aligned::order_type::add;
    aligned::message_t new_msg1{};
    new_msg1 = msg;
    messages_.emplace_back(new_msg1);
  }
  else if (msg.quantity != prev_msg.quantity &&  msg.price == prev_msg.price)
  {
    // send modify message with same order id
    msg.order_id = prev_msg.order_id;
    msg.type = aligned::order_type::modify;
    aligned::message_t new_msg1{};
    new_msg1 = msg;
    messages_.emplace_back(new_msg1);
  }
}

void fake_gateway_in::release_next_update()
{
  if ((get_time() - last_release_time_ >= throttle_) )
  {
    aligned::message_t update = next_update();
    update.price += price_offset_;
    update.entry_time = get_time();

    to_bk_buffer_.push_back(update);
    last_release_time_ = get_time();
  }
}

void fake_gateway_in::in_main_loop()
{
  while (ready_)
    while(on_)
    {
      for (auto update : messages_)
      {
        update.price += price_offset_;
        update.entry_time = get_time();
        if (update.type != aligned::order_type::add &&
          update.type != aligned::order_type::modify &&
          update.type != aligned::order_type::delete_ )
          std::cout << "ORDER TYPE IS NONE (fake in)" << std::endl;
        to_bk_buffer_.push_back(update);
        std::this_thread::sleep_for(std::chrono::milliseconds(throttle_));
      }
      //turn_off();
    }
}


fake_gateway_out::fake_gateway_out(unsigned long throttle,
                                   aligned::gw_to_om_buffer &to_om_buffer,
                                   aligned::om_to_gw_buffer &to_gw_buffer)
  : throttle_(throttle), to_om_buffer_(to_om_buffer),
    to_gw_buffer_(to_gw_buffer)
{}

void fake_gateway_out::respond()
{
  using namespace aligned;
  if (!to_gw_buffer_.empty())
  {
    auto to_gw = to_gw_buffer_.pop_front();
    aligned::from_gateway_out_t from_gw{};
    from_gw.price = to_gw.price;
    from_gw.quantity = to_gw.quantity;
    from_gw.status = status_type::filled;
    from_gw.internal_order_id = to_gw.internal_order_id;
    from_gw.side = to_gw.side;
    from_gw.symbol = to_gw.symbol;
    from_gw.venue = to_gw.venue;
    from_gw.event_time = get_time();

    if (to_gw.type == order_type::market)
    {
      if (get_time() % 2 == 0)
      {
        int scalar = (from_gw.side == side_type::ask_side) ? -1 : 1;
        from_gw.price += 1 * scalar;
      }
    }
    if (to_gw.type == order_type::ioc)
    {
      if (get_time() % 2 == 0)
        if (from_gw.quantity > 3)
          from_gw.quantity -= get_time() % 3;
    }
    if (to_gw.type == order_type::cancel)
      from_gw.status = status_type::cancelled;

    if (to_gw.type == order_type::limit)
      from_gw.status = status_type::accepted;

    to_om_buffer_.push_back(from_gw);
  }
}

void fake_gateway_out::release_response()
{
  if (get_time() - last_release_time_ >= throttle_)
  {
    respond();
    last_release_time_ = get_time();
  }
}

void fake_gateway_out::out_main_loop()
{
  while (on_)
  {
    release_response();
  }
}

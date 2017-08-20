//
// Created by eli on 8/19/17.
//

#ifndef TRADING_SYSTEM_DEMO_FAKE_GATEWAY_HPP
#define TRADING_SYSTEM_DEMO_FAKE_GATEWAY_HPP

#include <vector>
#include <iostream>
#include <iterator>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <chrono>
#include "ipc_messages.hpp"
#include "buffer_types.hpp"
//#include "trading_system_component.hpp"

class csv_row;

class fake_gateway_in
{
public:
  fake_gateway_in(
    aligned::symbol_name symbol, aligned::venue_name venue,
    std::string data_file_name, unsigned long throttle_milli,
    aligned::gw_to_bk_buffer& to_bk_buffer
  );

  void set_price_offset(unsigned px_offset) { price_offset_ = px_offset; }

  void release_next_update();

  aligned::message_t next_update() { return messages_[++time_series_idx_]; }

  const std::size_t message_count() const { return messages_.size(); }

private:
  std::pair<aligned::message_t, aligned::message_t> make_message(csv_row row);

  void add_modify_delete_message(
    aligned::message_t& msg, aligned::message_t& prev_msg);

  aligned::aligned_t new_id() { return ++order_id_; }

  aligned::aligned_t get_time()
  {
    auto time_now = std::chrono::high_resolution_clock::now().time_since_epoch();
    return static_cast<aligned::aligned_t>(time_now.count());
  }

  aligned::symbol_name symbol_;
  aligned::venue_name venue_;
  std::string data_file_name_;
  unsigned long throttle_;
  std::vector<aligned::message_t> messages_;
  int time_series_idx_{-1};
  aligned::aligned_t price_offset_{0};
  aligned::gw_to_bk_buffer& to_bk_buffer_;
  aligned::aligned_t order_id_{0};
  aligned::aligned_t last_release_time_{0};
};


class fake_gateway_out
{
public:
  explicit fake_gateway_out(
    unsigned long throttle, aligned::gw_to_om_buffer& to_om_buffer,
    aligned::om_to_gw_buffer& to_gw_buffer
  );

  void respond();

  void release_response();
private:
  aligned::aligned_t get_time()
  {
    auto time_now = std::chrono::high_resolution_clock::now().time_since_epoch();
    return static_cast<aligned::aligned_t>(time_now.count());
  }

  unsigned long throttle_;
  aligned::gw_to_om_buffer& to_om_buffer_;
  aligned::om_to_gw_buffer& to_gw_buffer_;
  aligned::aligned_t last_release_time_{0};
};




// csv read class for fake gateway in
class csv_row
{
public:
  const aligned::aligned_t& operator[](std::size_t index) const
  { return data_[index]; }

  std::size_t size() const { return data_.size(); }

  void read_next_row(std::istream& str)
  {
    std::string         line;
    std::getline(str, line);
    std::stringstream   lineStream(line);
    std::string         cell;
    data_.clear();
    while(std::getline(lineStream, cell, ','))
    {
      using namespace aligned;
      auto m_val = static_cast<aligned_t>(std::stof(cell));
      data_.push_back(m_val);
    }
  }

private:
  std::vector<aligned::aligned_t> data_;
};



#endif //TRADING_SYSTEM_DEMO_FAKE_GATEWAY_HPP

//
// Created by eli on 8/8/17.
//

#ifndef TRADING_SYSTEM_COMPONENTS_TRADING_SYSTEM_COMPONENT_HPP
#define TRADING_SYSTEM_COMPONENTS_TRADING_SYSTEM_COMPONENT_HPP

#include <chrono>
#include <iostream>
#include "ipc_messages.hpp"
#include <condition_variable>
#include <mutex>
#include <thread>
#include "ipc_messages.hpp"
#include "aligned_circular_buffer.hpp"
#include "buffer_types.hpp"

// for MongoDB
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/json.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/stdx.hpp>
#include <mongocxx/uri.hpp>
#include <bsoncxx/types.hpp>

using bsoncxx::builder::stream::close_array;
using bsoncxx::builder::stream::close_document;
using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;
using bsoncxx::builder::stream::open_array;
using bsoncxx::builder::stream::open_document;


template<typename T>
class trading_system_component
{
public:
  using tsc_type = std::uint64_t ;

  explicit trading_system_component(tsc_type component_id)
    : component_id_(component_id), component_ready_(false),
      shut_down_(false), log_file_name_("")
  {}

  ~trading_system_component() = default;
  trading_system_component(trading_system_component&) = delete;
  trading_system_component(trading_system_component&&) = delete;
  trading_system_component&operator=(trading_system_component&) = delete;
  trading_system_component&operator=(trading_system_component&&) = delete;

  virtual void component_main_loop()
  {
    T_this().component_main_loop();
  }

  virtual void handle_update()
  {
    T_this().handle_update();
  }

  virtual void handle_response()
  {
    T_this().handle_response();
  }

  virtual const bool is_ready()
  {
    return T_this().is_ready();
  }

  virtual void log_update()
  {
    T_this().log_update();
  }

  void declare_log_file(const std::string log_file_name)
  {
    log_file_name_ += log_file_name;
  }

  tsc_type id() { return component_id_; }

  // used by the viewer to wrap up tasks and shut down component
  void shut_down() { shut_down_ = true; }

  tsc_type get_time()
  {
    auto time_now = std::chrono::high_resolution_clock::now().time_since_epoch();
    return static_cast<tsc_type>(time_now.count());
  }


protected:
  T& T_this() { return *static_cast<T*>(this); }
  const T& T_this() const { return *static_cast<const T*>(this); }

  tsc_type component_id_ = 0;
  bool component_ready_;
  bool shut_down_;
  std::string log_file_name_;
  // will have buffers to receive and return messages unique to component
  // connection to MongoDB
  mongocxx::instance& inst_ = mongocxx::instance::current();
  mongocxx::client conn_{mongocxx::uri{}};
};

#endif //TRADING_SYSTEM_COMPONENTS_TRADING_SYSTEM_COMPONENT_HPP

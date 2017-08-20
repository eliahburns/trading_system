//
// Created by eli on 8/10/17.
//

#ifndef TRADING_SYSTEM_COMPONENTS_BUFFER_TYPES_HPP
#define TRADING_SYSTEM_COMPONENTS_BUFFER_TYPES_HPP

#include "ipc_messages.hpp"
#include "aligned_circular_buffer.hpp"


namespace aligned
{
  static const aligned_t k_buffer_size = 512;

  // strategy receives messages from book builder
  using bk_to_strat_buffer =
    aligned_circular_buffer<aligned::tob_t, k_buffer_size>;
  using gw_to_bk_buffer =
  aligned_circular_buffer<aligned::message_t,  k_buffer_size>;

  // order manager receives messages from trader
  // order manager sends messages to trader
  // order manager receives messages from gateway out
  // order manager sends messages to gateway out
  using om_to_gw_buffer =
    aligned_circular_buffer<aligned::to_gateway_out_t, k_buffer_size>;
  using gw_to_om_buffer =
    aligned_circular_buffer<aligned::from_gateway_out_t, k_buffer_size>;
  using om_to_strat_buffer =
    aligned_circular_buffer<aligned::from_gateway_out_t, k_buffer_size>;
  using strat_to_om_buffer =
    aligned_circular_buffer<aligned::to_gateway_out_t, k_buffer_size>;

  using tob_buffer_t =
    aligned_circular_buffer<aligned::tob_t, k_buffer_size>;
}

#endif //TRADING_SYSTEM_COMPONENTS_BUFFER_TYPES_HPP

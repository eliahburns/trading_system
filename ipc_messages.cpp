//
// Created by eli on 8/20/17.
//
#include "ipc_messages.hpp"


std::string aligned::symbol_to_str(aligned::symbol_name symbol_n)
{
  int symbol = 0;
  if (symbol_n == symbol_name::symbol_one)
    symbol = 1;
  if (symbol_n == symbol_name::symbol_two)
    symbol = 2;
  if (symbol_n == symbol_name::symbol_three)
    symbol = 3;
  if (symbol_n == symbol_name::symbol_four)
    symbol = 4;
  return std::to_string(symbol);
}

std::string aligned::venue_to_str(aligned::venue_name venue_n)
{
  int ven = 0;
  ven = (venue_n == venue_name::venue_one) ?  1 : 2;
  if (venue_n == venue_name::venue_three)
    ven = 3;
  if (venue_n == venue_name::venue_four)
    ven = 4;
  return std::to_string(ven);
}

**Example Trading System**

Here is a small example of a trading system, the code of which I wrote in the
span of around a couple of weeks in August. Its main purpose is to demonstrate
C++ code I've written that involve concepts/tasks such as:
* object oriented programming,
* curiously recurring template patterns
* trading system design
* low level optimizations, such as exploiting the alignment of data structures to account for cache block sizes (e.g. aligned_circular_buffer)
* working with NoSQL databases to keep track of outstanding orders and positions through viewers

Keep in mind though that this is a toy example. The strategy, which I call arbitrage_trader,
is mainly a placeholder--it simply waits for a profitable price discrepancy to occur and then sends
corresponding orders to each market.

You'll see that I have a base CRTP class called trading_system_component. All other
main classes of the trading system inherit from this class. As an example of CRTP multilevel inheritance, take a look
at the files trading_system_component.hpp, strategy.hpp, and arbitrage_trader.hpp

In the test folder, there are a larger number of unit tests.
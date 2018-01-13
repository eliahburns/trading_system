# Usage

<img align="right" width="250" height="175"
     src="data/market.jpg">

Firstly, I recommend not attempting to clone this repository and using it as
is to make money--this will very likely not work. Although some of the interfaces
to different parts are somewhat extensive, I can think of many features that
could still be added, particularly in terms of the **arbitrage\_trader** class--this
strategy could never really be expected to make money with its current logic
unless we were the fastest participant in the market.

This repository's primary purpose
is an example of how a given trading system could be designed in an object
oriented fashion, using C++11, and implementing a few optimizations.

There are two intended executables defined in the CMake file. One for the
unit tests located in the test directory. And another executable that runs
the trading system on some simulated data--it's main file is located in the
example directory.

# Overall Design

The main base class, from which all other primary pieces of the trading system
inherit from, is called **trading_system_component**. In its header file we
see a basic interface with methods that we would like use in all other pieces
of our trading system in order to have some consistency throughout their various
interfaces. Now, if you've never used or written code that employs
[CRTP](https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern),
you may be a little confused by what we're doing and for what purpose.

The most essential piece to understand involves **static_cast**:

```cpp
  T& T_this() { return *static_cast<T*>(this); }
```
The above line is using *static_cast* to recast the *trading_system_component*
to a pointer of the type that it takes as a template parameter. It then
dereferences that pointer and returns it, allowing us to use this function
to make calls to the object of the underlying type, such as in the component
main loop:

```cpp
  virtual void component_main_loop()
  {
    T_this().component_main_loop();
  }
```
Why do we want to do something that introduces that amount of complexity when
we could employ a more simple pure virtual base class? In a nutshell, it allows
us to employ an object oriented design pattern while avoid the costs associated
with virtual function table lookups--having the actual function available at
compile time also allows the compiler to make optimizations at the assembly
level that it otherwise could not.

We are not limited to single level inheritance with this technique either. We
can build classes that use the same technique on top of other classes using
CRTP and employ the same technique, allowing us to maintain an object oriented
design scheme without any of the performance costs. There's an example of
multilevel inheritance using CRTP in this project associated with the example
strategy:

```cpp
template<typename T>
class trading_system_component
{
...
};

template <typename T>
class strategy : public trading_system_component<T>
{
...
};

class arbitrage_trader : public strategy<arbitrage_trader>
{
...
};
```
This could continue indefinitely. For example we could have made **arbitrage_trader**
into a template class and then inherited from that to make various specialized
arbitrage trader classes.

<br><br>

There are three main classes that directly inherit from **trading_system_component**.
They are **book_builder**, **strategy**, and **order_manager**. Each of these
classes would ideally be running on their own process and communicate when needed
through buffers by acting as producers or consumers of information, given the
context. The *component\_main\_loop*, defined in each class, can be launched from
a thread and run indefinitely until the user shuts the system down.

##### book_builder class:

This class builds out a book of a given market using two instances of the
**book** class--one for the bid side and one for the ask side. It's main
loop would continuously check for new messages relating market updates, passed
to it from class maintaining a connection to a market gateway. As updates
are received, it adjusts it's representation of the market and then passes
on any information requested by the strategy.

It's likely that any strategy we come up with with be quite concerned with
what the price of inside market (best bid/ask) is at any given time. In
order to be able to access this information quickly, we must consider what
time of data structure our book should have. In my implementation of C++, the
STL [map](http://www.cplusplus.com/reference/map/map/) class is a binary
search tree. We can use this with linked lists to get O(1) time look ups
for the best bid and best ask with some small customizations for whether
the book is composed of bids or asks.

The objects of type bid and ask used by the book class can be found in the
*quotes* header file.


##### strategy class:

The **strategy** class has a main loop that checks for updates from certain
markets and then uses a variety of functions, including a signal and execution
to then potentially act on such updates.

This class isn't expected to be the end of the line of a strategy implementation
so it again takes another type in a template parameter is somewhat generic, although
more specialized than our base class, *trading\_system\_component*. However, there
are some implemented functions in the strategy class that we can expect any
specialization to want to use, such as the current pnl, the current unrealized pnl,
the ability to adjust its quotes, among others.

##### order_manager class:

The order manager is essentially the gate keeper between a given specialized strategy
and and the connection to a given exchange that we send new order requests and cancels
to. It sits in its main process and should have certain safety checks to hopefully keep
our strategy out of trouble. In can provide certain risk checks to make sure that a given
strategy is staying within its predefined bounds. One particularly important feature is
a throttle used to prevent a strategy from sending out bursts of orders that may not be
intended before we can react and turn it off or adjust parameters.

## Inter Process Communication

I've included a file of a circular buffer implementation of mine, a header
file, *buffer_types.hpp*, that defines types of different buffers to be used between specific
components of the trading system, and another header file, *ipc_messages.hpp*,
that has various messages define that used between producers and consumers passing
messages between each other using a given circular buffer.

## Requirements

Currently, everything should be self contained in this directory except
for MongoDB. If you want to clone this repository and run the tests you need
to have this installed. If you look in *CMakeLists.txt* you will see there
are paths defined where these are located on my Linux machine--you may have
to modify these in order compile this project, as well. I'm considering removing
this to make it easier to run the unit tests, but doing so would eliminate all
examples of logging events with the trading system.




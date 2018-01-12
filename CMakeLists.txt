cmake_minimum_required(VERSION 3.5)
project(trading_system)

#find_package(libmongocxx REQUIRED)
#find_package(libbsoncxx REQUIRED)

include_directories("/usr/local/lib")
include_directories("/usr/local/include/mongocxx/v_noabi/")
include_directories("/usr/local/include/bsoncxx/v_noabi/")

include_directories("../Catch/single_include/")

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_EXE_LINKER_FLAGS
        "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath -Wl,/usr/local/lib")

set(SOURCE_FILES main.cpp buffer_types.hpp aligned_circular_buffer.hpp quote.hpp
        ipc_messages.hpp book.hpp trading_system_component.hpp order_manager.hpp
        book_builder.hpp strategy.hpp arbitrage_trader.hpp book_builder.cpp
        arbitrage_trader.cpp trading_system_viewer.cpp trading_system_viewer.hpp
        order_manager.cpp fake_gateway.hpp fake_gateway.cpp
        strategy.cpp ipc_messages.cpp test/test_bb_with_data.cpp barrier.hpp)

add_executable(trading_system ${SOURCE_FILES})

target_link_libraries (trading_system pthread)
target_link_libraries(trading_system ${LIBMONGOCXX_LIBRARIES})
target_link_libraries(trading_system ${LIBBSONCXX_LIBRARIES})

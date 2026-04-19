cmake_minimum_required(VERSION 3.16)
project(orderbook LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(orderbook_core
    src/analytics.cpp
    src/exchange.cpp
    src/order.cpp
    src/order_book.cpp
    src/matching_engine.cpp
    src/reporting.cpp
    src/simulator.cpp
    src/strategy.cpp
    src/utils.cpp
)

target_include_directories(orderbook_core PUBLIC include)

add_executable(orderbook_demo src/main.cpp)
target_link_libraries(orderbook_demo PRIVATE orderbook_core)

add_executable(strategy_demo src/strategy_main.cpp)
target_link_libraries(strategy_demo PRIVATE orderbook_core)

add_executable(benchmark benchmarks/benchmark.cpp)
target_link_libraries(benchmark PRIVATE orderbook_core)

add_executable(test_basic tests/test_basic.cpp)
target_link_libraries(test_basic PRIVATE orderbook_core)

add_executable(test_matching tests/test_matching.cpp)
target_link_libraries(test_matching PRIVATE orderbook_core)

add_executable(test_cancel tests/test_cancel.cpp)
target_link_libraries(test_cancel PRIVATE orderbook_core)

add_executable(test_partial_fill tests/test_partial_fill.cpp)
target_link_libraries(test_partial_fill PRIVATE orderbook_core)

add_executable(test_simulator tests/test_simulator.cpp)
target_link_libraries(test_simulator PRIVATE orderbook_core)

add_executable(test_analytics tests/test_analytics.cpp)
target_link_libraries(test_analytics PRIVATE orderbook_core)

add_executable(test_strategy tests/test_strategy.cpp)
target_link_libraries(test_strategy PRIVATE orderbook_core)

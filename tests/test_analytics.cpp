#include "analytics.h"
#include "simulator.h"

#include <cassert>
#include <cmath>
#include <iostream>

namespace {

void test_top_of_book_depth_and_imbalance_metrics() {
    ob::MatchingEngine engine;
    engine.submit_limit_order(1, ob::Side::Buy, 100, 10);
    engine.submit_limit_order(2, ob::Side::Buy, 99, 20);
    engine.submit_limit_order(3, ob::Side::Sell, 101, 15);
    engine.submit_limit_order(4, ob::Side::Sell, 102, 5);

    const auto market_metrics = ob::Analytics::compute_market_metrics(engine.order_book(), 2);
    assert(market_metrics.top_of_book.best_bid.value() == 100);
    assert(market_metrics.top_of_book.best_ask.value() == 101);
    assert(market_metrics.top_of_book.spread.value() == 1);
    assert(std::abs(market_metrics.top_of_book.mid_price.value() - 100.5) < 1e-9);
    assert(market_metrics.depth.bid_top_n_volume == 30);
    assert(market_metrics.depth.ask_top_n_volume == 20);
    assert(market_metrics.depth.bid_cumulative_depth.size() == 2);
    assert(market_metrics.depth.bid_cumulative_depth[0] == 10);
    assert(market_metrics.depth.bid_cumulative_depth[1] == 30);
    assert(market_metrics.imbalance.total_bid_volume == 30);
    assert(market_metrics.imbalance.total_ask_volume == 20);
}

void test_trade_and_execution_metrics() {
    ob::MatchingEngine engine;
    engine.submit_limit_order(10, ob::Side::Sell, 101, 4);
    engine.submit_limit_order(11, ob::Side::Sell, 102, 6);

    const ob::Event event {
        .timestamp = 1,
        .type = ob::EventType::Add,
        .order_id = 12,
        .side = ob::Side::Buy,
        .price = 102,
        .quantity = 8,
        .raw_command = "1,ADD,BUY,12,102,8",
    };

    const auto before_top = engine.order_book().top_of_book();
    const auto result = engine.submit_limit_order(12, ob::Side::Buy, 102, 8);
    const auto after_top = engine.order_book().top_of_book();

    const auto trade_metrics = ob::Analytics::compute_trade_metrics(result.trades);
    assert(trade_metrics.traded_volume == 8);
    assert(trade_metrics.execution_count == 2);
    assert(std::abs(trade_metrics.vwap.value() - 101.5) < 1e-9);
    assert(std::abs(trade_metrics.average_trade_size.value() - 4.0) < 1e-9);

    const auto execution_metrics =
        ob::Analytics::compute_execution_metrics(event, result, before_top, after_top);
    assert(std::abs(execution_metrics.fill_ratio - 1.0) < 1e-9);
    assert(std::abs(execution_metrics.slippage.value() - 0.5) < 1e-9);
    assert(execution_metrics.queue_depletion == 4);
}

}  // namespace

int main() {
    test_top_of_book_depth_and_imbalance_metrics();
    test_trade_and_execution_metrics();
    std::cout << "test_analytics passed.\n";
    return 0;
}

#include "simulator.h"
#include "strategy.h"

#include <cassert>
#include <iostream>
#include <sstream>

namespace {

void test_imbalance_strategy_generates_activity_and_tracks_stats() {
    std::istringstream input(
        "timestamp,symbol,type,side,order_id,price,quantity\n"
        "1,TEST,ADD,BUY,1,100,50\n"
        "2,TEST,ADD,BUY,2,99,40\n"
        "3,TEST,ADD,SELL,3,101,10\n"
        "4,TEST,ADD,SELL,4,101,10\n"
        "5,TEST,ADD,BUY,5,100,30\n");

    ob::Simulator simulator;
    const auto events = simulator.load_events(input);

    ob::ImbalanceTakerStrategy strategy({.imbalance_threshold = 0.50, .order_quantity = 5});
    const auto result = strategy.run(events);

    assert(result.stats.external_events_processed == events.size());
    assert(result.stats.signals_triggered > 0);
    assert(result.stats.orders_sent > 0);
    assert(result.stats.requested_quantity == result.stats.orders_sent * 5);
    assert(result.stats.filled_quantity > 0);
    assert(result.stats.fill_ratio > 0.0);
    assert(!result.fills.empty());
}

}  // namespace

int main() {
    test_imbalance_strategy_generates_activity_and_tracks_stats();
    std::cout << "test_strategy passed.\n";
    return 0;
}

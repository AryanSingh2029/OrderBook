#include "matching_engine.h"

#include <cassert>
#include <iostream>

namespace {

void test_cancel_removes_order() {
    ob::MatchingEngine engine;
    engine.submit_limit_order(10, ob::Side::Buy, 99, 50);
    engine.submit_limit_order(11, ob::Side::Buy, 99, 20);

    assert(engine.cancel_order(10));
    assert(!engine.cancel_order(10));
    assert(engine.order_book().volume_at_price(ob::Side::Buy, 99).value() == 20);
    assert(engine.order_book().order_count() == 1);
    const auto state = engine.order_state(10);
    assert(state.has_value());
    assert(state->status == ob::OrderStatus::Cancelled);
}

void test_cancel_already_filled_order() {
    ob::MatchingEngine engine;
    engine.submit_limit_order(20, ob::Side::Buy, 100, 5);
    engine.submit_limit_order(21, ob::Side::Sell, 100, 5);

    assert(!engine.cancel_order(20));
    const auto state = engine.order_state(20);
    assert(state.has_value());
    assert(state->status == ob::OrderStatus::Filled);
}

void test_cancel_nonexistent_order() {
    ob::MatchingEngine engine;
    assert(!engine.cancel_order(9999));
    assert(!engine.order_state(9999).has_value());
}

}  // namespace

int main() {
    test_cancel_removes_order();
    test_cancel_already_filled_order();
    test_cancel_nonexistent_order();
    std::cout << "test_cancel passed.\n";
    return 0;
}

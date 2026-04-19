#include "matching_engine.h"

#include <cassert>
#include <iostream>
#include <stdexcept>

namespace {

void test_add_one_buy_order() {
    ob::MatchingEngine engine;
    engine.submit_limit_order(1, ob::Side::Buy, 100, 10);

    const auto top = engine.order_book().top_of_book();
    assert(top.best_bid.has_value());
    assert(!top.best_ask.has_value());
    assert(top.best_bid->price == 100);
    assert(top.best_bid->quantity == 10);
    assert(engine.order_book().order_count() == 1);
}

void test_add_one_sell_order() {
    ob::MatchingEngine engine;
    engine.submit_limit_order(2, ob::Side::Sell, 105, 8);

    const auto top = engine.order_book().top_of_book();
    assert(!top.best_bid.has_value());
    assert(top.best_ask.has_value());
    assert(top.best_ask->price == 105);
    assert(top.best_ask->quantity == 8);
    assert(engine.order_book().order_count() == 1);
}

void test_confirm_book_state_with_both_sides() {
    ob::MatchingEngine engine;
    engine.submit_limit_order(1, ob::Side::Buy, 100, 10);
    engine.submit_limit_order(2, ob::Side::Buy, 101, 5);
    engine.submit_limit_order(3, ob::Side::Sell, 105, 8);

    const auto top = engine.order_book().top_of_book();
    assert(top.best_bid.has_value());
    assert(top.best_ask.has_value());
    assert(top.best_bid->price == 101);
    assert(top.best_bid->quantity == 5);
    assert(top.best_ask->price == 105);
    assert(engine.order_book().spread().value() == 4);
}

void test_zero_quantity_rejection() {
    ob::MatchingEngine engine;

    bool threw = false;
    try {
        engine.submit_limit_order(10, ob::Side::Buy, 100, 0);
    } catch (const std::invalid_argument&) {
        threw = true;
    }

    assert(threw);
    const auto state = engine.order_state(10);
    assert(state.has_value());
    assert(state->status == ob::OrderStatus::Rejected);
}

void test_invalid_price_rejection() {
    ob::MatchingEngine engine;

    bool threw = false;
    try {
        engine.submit_limit_order(11, ob::Side::Sell, 0, 5);
    } catch (const std::invalid_argument&) {
        threw = true;
    }

    assert(threw);
    const auto state = engine.order_state(11);
    assert(state.has_value());
    assert(state->status == ob::OrderStatus::Rejected);
}

void test_duplicate_order_id_rejection() {
    ob::MatchingEngine engine;
    engine.submit_limit_order(12, ob::Side::Buy, 100, 5);

    bool threw = false;
    try {
        engine.submit_limit_order(12, ob::Side::Sell, 101, 4);
    } catch (const std::invalid_argument&) {
        threw = true;
    }

    assert(threw);
    assert(engine.order_book().order_count() == 1);
}

}  // namespace

int main() {
    test_add_one_buy_order();
    test_add_one_sell_order();
    test_confirm_book_state_with_both_sides();
    test_zero_quantity_rejection();
    test_invalid_price_rejection();
    test_duplicate_order_id_rejection();
    std::cout << "test_basic passed.\n";
    return 0;
}

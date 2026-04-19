#include "matching_engine.h"

#include <cassert>
#include <iostream>

namespace {

void test_full_fill() {
    ob::MatchingEngine engine;
    engine.submit_limit_order(1, ob::Side::Sell, 101, 10);

    const auto result = engine.submit_limit_order(2, ob::Side::Buy, 101, 10);
    assert(result.trades.size() == 1);
    assert(result.summary.executed_quantity == 10);
    assert(result.summary.remaining_quantity == 0);
    assert(engine.order_book().order_count() == 0);
}

void test_partial_fill() {
    ob::MatchingEngine engine;
    engine.submit_limit_order(10, ob::Side::Sell, 101, 10);

    const auto result = engine.submit_limit_order(11, ob::Side::Buy, 101, 6);
    assert(result.trades.size() == 1);
    assert(result.trades[0].trade_quantity == 6);
    assert(result.summary.executed_quantity == 6);
    assert(engine.order_book().volume_at_price(ob::Side::Sell, 101).value() == 4);
}

void test_multiple_matches_across_levels() {
    ob::MatchingEngine engine;
    engine.submit_limit_order(20, ob::Side::Sell, 101, 5);
    engine.submit_limit_order(21, ob::Side::Sell, 102, 7);
    engine.submit_limit_order(22, ob::Side::Sell, 103, 9);

    const auto result = engine.submit_limit_order(23, ob::Side::Buy, 102, 12);
    assert(result.trades.size() == 2);
    assert(result.trades[0].sell_order_id == 20);
    assert(result.trades[0].trade_price == 101);
    assert(result.trades[0].trade_quantity == 5);
    assert(result.trades[1].sell_order_id == 21);
    assert(result.trades[1].trade_price == 102);
    assert(result.trades[1].trade_quantity == 7);
    assert(!engine.order_book().volume_at_price(ob::Side::Sell, 101).has_value());
    assert(!engine.order_book().volume_at_price(ob::Side::Sell, 102).has_value());
    assert(engine.order_book().volume_at_price(ob::Side::Sell, 103).value() == 9);
}

void test_fifo_at_same_price() {
    ob::MatchingEngine engine;
    engine.submit_limit_order(30, ob::Side::Sell, 101, 10);
    engine.submit_limit_order(31, ob::Side::Sell, 101, 7);

    const auto first_position = engine.order_book().queue_position(30);
    const auto second_position = engine.order_book().queue_position(31);
    assert(first_position.has_value());
    assert(second_position.has_value());
    assert(first_position->orders_ahead == 0);
    assert(first_position->quantity_ahead == 0);
    assert(second_position->orders_ahead == 1);
    assert(second_position->quantity_ahead == 10);

    const auto result = engine.submit_limit_order(32, ob::Side::Buy, 102, 12);
    assert(result.trades.size() == 2);
    assert(result.trades[0].buy_order_id == 32);
    assert(result.trades[0].sell_order_id == 30);
    assert(result.trades[0].trade_quantity == 10);
    assert(result.trades[1].buy_order_id == 32);
    assert(result.trades[1].sell_order_id == 31);
    assert(result.trades[1].trade_quantity == 2);
    assert(result.summary.executed_quantity == 12);
    assert(result.summary.remaining_quantity == 0);
    assert(engine.order_book().volume_at_price(ob::Side::Sell, 101).value() == 5);
    const auto remaining_position = engine.order_book().queue_position(31);
    assert(remaining_position.has_value());
    assert(remaining_position->orders_ahead == 0);
    assert(remaining_position->quantity_ahead == 0);
}

void test_market_order_never_rests() {
    ob::MatchingEngine engine;
    engine.submit_limit_order(10, ob::Side::Sell, 101, 4);
    const auto result = engine.submit_market_order(11, ob::Side::Buy, 10);

    assert(result.summary.executed_quantity == 4);
    assert(result.summary.remaining_quantity == 6);
    assert(!engine.order_book().top_of_book().best_ask.has_value());
    assert(engine.order_book().order_count() == 0);
}

void test_market_order_on_empty_book() {
    ob::MatchingEngine engine;
    const auto result = engine.submit_market_order(40, ob::Side::Buy, 10);

    assert(result.trades.empty());
    assert(result.summary.executed_quantity == 0);
    assert(result.summary.remaining_quantity == 10);
    assert(engine.order_book().order_count() == 0);
}

}  // namespace

int main() {
    test_full_fill();
    test_partial_fill();
    test_multiple_matches_across_levels();
    test_fifo_at_same_price();
    test_market_order_never_rests();
    test_market_order_on_empty_book();
    std::cout << "test_matching passed.\n";
    return 0;
}

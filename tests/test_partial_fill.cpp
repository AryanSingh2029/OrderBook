#include "matching_engine.h"

#include <cassert>
#include <iostream>
#include <stdexcept>

namespace {

void test_modify_reprices_order() {
    ob::MatchingEngine engine;
    engine.submit_limit_order(1, ob::Side::Buy, 100, 10);
    engine.submit_limit_order(2, ob::Side::Sell, 103, 6);

    const auto result = engine.replace_order(1, 104, 10);
    const auto top = engine.order_book().top_of_book();
    assert(top.best_bid.has_value());
    assert(!top.best_ask.has_value());
    assert(top.best_bid->price == 104);
    assert(top.best_bid->quantity == 4);
    assert(engine.order_book().order_count() == 1);
    assert(!result.trades.empty());
}

void test_replace_to_zero_equivalent_to_cancel() {
    ob::MatchingEngine engine;
    engine.submit_limit_order(55, ob::Side::Sell, 110, 9);
    const auto result = engine.replace_order(55, 110, 0);
    assert(result.trades.empty());
    assert(result.summary.requested_quantity == 0);
    assert(result.summary.inserted_into_book == false);
    assert(engine.order_book().order_count() == 0);
    assert(!engine.order_book().top_of_book().best_ask.has_value());
}

void test_replace_with_invalid_price_rejected_without_losing_order() {
    ob::MatchingEngine engine;
    engine.submit_limit_order(56, ob::Side::Sell, 110, 9);

    bool threw = false;
    try {
        (void)engine.replace_order(56, 0, 9);
    } catch (const std::invalid_argument&) {
        threw = true;
    }

    assert(threw);
    assert(engine.order_book().order_count() == 1);
    assert(engine.order_book().volume_at_price(ob::Side::Sell, 110).value() == 9);
}

void test_replace_quantity_change_updates_book_state() {
    ob::MatchingEngine engine;
    engine.submit_limit_order(57, ob::Side::Buy, 100, 9);

    const auto result = engine.replace_order(57, 100, 15);
    const auto metadata = engine.order_book().order_metadata(57);
    assert(metadata.has_value());
    assert(result.summary.requested_quantity == 15);
    assert(result.summary.executed_quantity == 0);
    assert(result.summary.remaining_quantity == 15);
    assert(result.summary.inserted_into_book);
    assert(metadata->price == 100);
    assert(metadata->remaining_quantity == 15);
}

void test_replace_reinserts_with_fresh_priority() {
    ob::MatchingEngine engine;
    engine.submit_limit_order(60, ob::Side::Sell, 101, 5);
    engine.submit_limit_order(61, ob::Side::Sell, 101, 5);

    const auto before = engine.order_book().order_metadata(60);
    assert(before.has_value());

    const auto replace_result = engine.replace_order(60, 101, 5);
    const auto after = engine.order_book().order_metadata(60);
    assert(after.has_value());
    assert(after->timestamp > before->timestamp);
    assert(replace_result.summary.inserted_into_book);
    const auto queue_position = engine.order_book().queue_position(60);
    assert(queue_position.has_value());
    assert(queue_position->orders_ahead == 1);
    assert(queue_position->quantity_ahead == 5);

    const auto aggressive = engine.submit_limit_order(62, ob::Side::Buy, 101, 6);
    assert(aggressive.trades.size() == 2);
    assert(aggressive.trades[0].sell_order_id == 61);
    assert(aggressive.trades[1].sell_order_id == 60);
}

}  // namespace

int main() {
    test_modify_reprices_order();
    test_replace_to_zero_equivalent_to_cancel();
    test_replace_with_invalid_price_rejected_without_losing_order();
    test_replace_quantity_change_updates_book_state();
    test_replace_reinserts_with_fresh_priority();
    std::cout << "test_partial_fill passed.\n";
    return 0;
}

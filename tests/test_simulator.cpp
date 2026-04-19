#include "simulator.h"

#include <cassert>
#include <iostream>
#include <sstream>

namespace {

void test_csv_event_stream_replays_and_updates_book() {
    std::istringstream input(
        "timestamp,symbol,type,side,order_id,price,quantity\n"
        "1,AAPL,ADD,BUY,101,100,250\n"
        "2,AAPL,ADD,SELL,102,50,249\n"
        "3,AAPL,ADD,SELL,103,60,250\n"
        "4,AAPL,CANCEL,,101,,\n");

    ob::Simulator simulator;
    ob::Exchange exchange;
    const auto events = simulator.load_events(input);
    assert(events.size() == 4);
    assert(events[0].timestamp == 1);
    assert(events[0].symbol == "AAPL");
    assert(events[3].type == ob::EventType::Cancel);

    const auto results = simulator.replay(events, exchange);
    assert(results.size() == 4);
    assert(results[2].routed_result.engine_result.trades.size() == 1);
    assert(results[2].routed_result.engine_result.trades[0].buy_order_id == 101);
    assert(results[2].routed_result.engine_result.trades[0].sell_order_id == 103);
    assert(results[2].routed_result.engine_result.trades[0].trade_price == 100);
    assert(!results[3].routed_result.cancel_succeeded);
    const auto* engine = exchange.engine_for("AAPL");
    assert(engine != nullptr);
    assert(engine->order_book().order_count() == 1);
}

void test_replay_is_deterministic_for_same_event_stream() {
    std::istringstream input(
        "timestamp,symbol,type,side,order_id,price,quantity\n"
        "1,AAPL,ADD,BUY,201,100,10\n"
        "2,AAPL,ADD,SELL,202,101,7\n"
        "3,AAPL,ADD,SELL,203,100,4\n"
        "4,AAPL,ADD,BUY,204,102,5\n"
        "5,AAPL,CANCEL,,202,,\n");

    ob::Simulator simulator;
    const auto events = simulator.load_events(input);

    ob::Exchange first_exchange;
    ob::Exchange second_exchange;
    const auto first_results = simulator.replay(events, first_exchange);
    const auto second_results = simulator.replay(events, second_exchange);
    const auto* first_engine = first_exchange.engine_for("AAPL");
    const auto* second_engine = second_exchange.engine_for("AAPL");
    assert(first_engine != nullptr);
    assert(second_engine != nullptr);

    assert(first_results.size() == second_results.size());
    assert(first_engine->order_book().order_count() == second_engine->order_book().order_count());
    assert(first_engine->order_book().top_of_book().best_bid.has_value() == second_engine->order_book().top_of_book().best_bid.has_value());
    assert(first_engine->order_book().top_of_book().best_ask.has_value() == second_engine->order_book().top_of_book().best_ask.has_value());

    const auto first_top = first_engine->order_book().top_of_book();
    const auto second_top = second_engine->order_book().top_of_book();
    if (first_top.best_bid) {
        assert(first_top.best_bid->price == second_top.best_bid->price);
        assert(first_top.best_bid->quantity == second_top.best_bid->quantity);
    }
    if (first_top.best_ask) {
        assert(first_top.best_ask->price == second_top.best_ask->price);
        assert(first_top.best_ask->quantity == second_top.best_ask->quantity);
    }

    std::size_t first_trade_count = 0;
    std::size_t second_trade_count = 0;
    for (const auto& result : first_results) {
        first_trade_count += result.routed_result.engine_result.trades.size();
    }
    for (const auto& result : second_results) {
        second_trade_count += result.routed_result.engine_result.trades.size();
    }
    assert(first_trade_count == second_trade_count);
}

void test_rejected_event_is_logged_and_replay_continues() {
    std::istringstream input(
        "timestamp,symbol,type,side,order_id,price,quantity\n"
        "1,AAPL,ADD,BUY,301,100,10\n"
        "2,AAPL,ADD,SELL,302,0,5\n"
        "3,AAPL,ADD,SELL,303,101,4\n");

    ob::Simulator simulator;
    ob::Exchange exchange;
    const auto events = simulator.load_events(input);
    const auto results = simulator.replay(events, exchange);

    assert(results.size() == 3);
    assert(!results[0].routed_result.rejected);
    assert(results[1].routed_result.rejected);
    assert(!results[1].routed_result.rejection_message.empty());
    assert(!results[2].routed_result.rejected);
    const auto* engine = exchange.engine_for("AAPL");
    assert(engine != nullptr);
    assert(engine->order_book().order_count() == 2);
}

void test_multi_instrument_replay_keeps_books_separate() {
    std::istringstream input(
        "timestamp,symbol,type,side,order_id,price,quantity\n"
        "1,AAPL,ADD,BUY,401,100,10\n"
        "2,MSFT,ADD,SELL,402,200,8\n"
        "3,AAPL,ADD,SELL,403,101,5\n"
        "4,MSFT,ADD,BUY,404,201,3\n");

    ob::Simulator simulator;
    ob::Exchange exchange;
    const auto events = simulator.load_events(input);
    const auto results = simulator.replay(events, exchange);

    assert(results.size() == 4);
    const auto* aapl = exchange.engine_for("AAPL");
    const auto* msft = exchange.engine_for("MSFT");
    assert(aapl != nullptr);
    assert(msft != nullptr);
    assert(aapl->order_book().top_of_book().best_bid.has_value());
    assert(aapl->order_book().top_of_book().best_bid->price == 100);
    assert(msft->order_book().top_of_book().best_ask.has_value());
    assert(msft->order_book().top_of_book().best_ask->price == 200);
}

}  // namespace

int main() {
    test_csv_event_stream_replays_and_updates_book();
    test_replay_is_deterministic_for_same_event_stream();
    test_rejected_event_is_logged_and_replay_continues();
    test_multi_instrument_replay_keeps_books_separate();
    std::cout << "test_simulator passed.\n";
    return 0;
}

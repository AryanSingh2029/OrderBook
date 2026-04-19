#include "simulator.h"

#include "utils.h"

#include <sstream>
#include <stdexcept>

namespace ob {

std::vector<Event> Simulator::load_events(std::istream& input) const {
    std::vector<Event> events;
    std::string line;
    bool first_line = true;
    Timestamp previous_timestamp = 0;

    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }

        const auto fields = split_csv_line(line);
        if (first_line && !fields.empty() && fields[0] == "timestamp") {
            first_line = false;
            continue;
        }
        first_line = false;

        if (fields.size() != 7) {
            throw std::runtime_error("expected 7 CSV fields per event");
        }

        const Timestamp timestamp = static_cast<Timestamp>(std::stoull(fields[0]));
        if (!events.empty() && timestamp < previous_timestamp) {
            throw std::runtime_error("events must be sorted by non-decreasing timestamp");
        }
        previous_timestamp = timestamp;

        const EventType type = parse_event_type(fields[2]);
        const Side side = is_blank(fields[3]) ? Side::Buy : parse_side(fields[3]);
        const Price price = is_blank(fields[5]) ? 0 : static_cast<Price>(std::stoll(fields[5]));
        const Quantity quantity = is_blank(fields[6]) ? 0 : static_cast<Quantity>(std::stoull(fields[6]));

        events.push_back(Event {
            .timestamp = timestamp,
            .symbol = fields[1],
            .type = type,
            .order_id = static_cast<OrderId>(std::stoull(fields[4])),
            .side = side,
            .price = price,
            .quantity = quantity,
            .raw_command = line,
        });
    }

    return events;
}

std::vector<Event> Simulator::load_commands(std::istream& input) const {
    std::vector<Event> events;
    std::string line;

    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }

        const auto tokens = split_whitespace(line);
        if (tokens.empty()) {
            continue;
        }

        if (tokens[0] == "CANCEL") {
            if (tokens.size() != 2) {
                throw std::runtime_error("CANCEL requires exactly 1 argument: order_id");
            }
            events.push_back(Event {
                .timestamp = static_cast<Timestamp>(events.size() + 1),
                .symbol = "DEFAULT",
                .type = EventType::Cancel,
                .order_id = static_cast<OrderId>(std::stoull(tokens[1])),
                .raw_command = line,
            });
            continue;
        }

        if (tokens.size() != 5) {
            throw std::runtime_error("order command must be: SIDE TYPE ORDER_ID PRICE QUANTITY");
        }

        events.push_back(Event {
            .timestamp = static_cast<Timestamp>(events.size() + 1),
            .symbol = "DEFAULT",
            .type = parse_event_type(tokens[1]),
            .order_id = static_cast<OrderId>(std::stoull(tokens[2])),
            .side = parse_side(tokens[0]),
            .price = static_cast<Price>(std::stoll(tokens[3])),
            .quantity = static_cast<Quantity>(std::stoull(tokens[4])),
            .raw_command = line,
        });
    }

    return events;
}

std::vector<ReplayResult> Simulator::replay(const std::vector<Event>& events, Exchange& exchange) const {
    std::vector<ReplayResult> results;
    results.reserve(events.size());

    for (const auto& event : events) {
        ReplayResult replay_result {.event = event};
        if (const auto* before_engine = exchange.engine_for(event.symbol)) {
            replay_result.pre_top_of_book = before_engine->order_book().top_of_book();
        }
        switch (event.type) {
            case EventType::Add:
                replay_result.routed_result =
                    exchange.submit_limit_order(event.symbol, event.order_id, event.side, event.price, event.quantity);
                break;
            case EventType::Market:
                replay_result.routed_result =
                    exchange.submit_market_order(event.symbol, event.order_id, event.side, event.quantity);
                break;
            case EventType::Cancel:
                replay_result.routed_result = exchange.cancel_order(event.order_id);
                break;
        }
        if (const auto* after_engine = exchange.engine_for(event.symbol)) {
            replay_result.top_of_book = after_engine->order_book().top_of_book();
            replay_result.bid_levels = after_engine->order_book().levels(Side::Buy, 5);
            replay_result.ask_levels = after_engine->order_book().levels(Side::Sell, 5);
            replay_result.market_metrics = Analytics::compute_market_metrics(after_engine->order_book(), 5);
        }
        replay_result.trade_metrics = Analytics::compute_trade_metrics(replay_result.routed_result.engine_result.trades);
        replay_result.execution_metrics = Analytics::compute_execution_metrics(
            event,
            replay_result.routed_result.engine_result,
            replay_result.pre_top_of_book,
            replay_result.top_of_book);
        results.push_back(replay_result);
    }

    return results;
}

std::string to_string(EventType type) {
    switch (type) {
        case EventType::Add:
            return "ADD";
        case EventType::Market:
            return "MARKET";
        case EventType::Cancel:
            return "CANCEL";
    }
    return "UNKNOWN";
}

}  // namespace ob

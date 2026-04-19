#pragma once

#include "analytics.h"
#include "exchange.h"

#include <istream>
#include <string>
#include <vector>

namespace ob {

enum class EventType {
    Add,
    Market,
    Cancel
};

struct Event {
    Timestamp timestamp {};
    std::string symbol;
    EventType type {EventType::Add};
    OrderId order_id {};
    Side side {Side::Buy};
    Price price {};
    Quantity quantity {};
    std::string raw_command;
};

struct ReplayResult {
    Event event;
    RoutedResult routed_result;
    TopOfBook pre_top_of_book;
    TopOfBook top_of_book;
    std::vector<LevelSnapshot> bid_levels;
    std::vector<LevelSnapshot> ask_levels;
    MarketMetrics market_metrics;
    TradeMetrics trade_metrics;
    ExecutionMetrics execution_metrics;
};

class Simulator {
public:
    std::vector<Event> load_events(std::istream& input) const;
    std::vector<Event> load_commands(std::istream& input) const;
    std::vector<ReplayResult> replay(const std::vector<Event>& events, Exchange& exchange) const;
};

[[nodiscard]] std::string to_string(EventType type);

}  // namespace ob

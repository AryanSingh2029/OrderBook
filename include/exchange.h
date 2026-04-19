#pragma once

#include "matching_engine.h"

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace ob {

struct RoutedResult {
    EngineResult engine_result;
    bool cancel_succeeded {};
    bool rejected {};
    std::string rejection_message;
};

class Exchange {
public:
    RoutedResult submit_limit_order(
        const std::string& symbol,
        OrderId id,
        Side side,
        Price price,
        Quantity quantity);
    RoutedResult submit_market_order(
        const std::string& symbol,
        OrderId id,
        Side side,
        Quantity quantity);
    RoutedResult cancel_order(OrderId id);

    [[nodiscard]] const MatchingEngine* engine_for(const std::string& symbol) const;
    [[nodiscard]] std::vector<std::string> symbols() const;
    [[nodiscard]] std::size_t total_resting_orders() const;

private:
    std::unordered_map<std::string, MatchingEngine> books_;
    std::unordered_map<OrderId, std::string> order_to_symbol_;
};

}  // namespace ob

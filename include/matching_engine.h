#pragma once

#include "order.h"
#include "order_book.h"

#include <optional>
#include <unordered_map>
#include <vector>

namespace ob {

enum class OrderStatus {
    Active,
    Filled,
    Cancelled,
    Replaced,
    Rejected
};

struct OrderState {
    OrderId order_id {};
    OrderStatus status {OrderStatus::Rejected};
    std::optional<Quantity> remaining_quantity;
};

struct EngineResult {
    AddResult summary;
    std::vector<Trade> trades;
    std::vector<OrderState> state_updates;
};

class MatchingEngine {
public:
    EngineResult submit_limit_order(OrderId id, Side side, Price price, Quantity quantity);
    EngineResult submit_market_order(OrderId id, Side side, Quantity quantity);
    EngineResult replace_order(OrderId id, Price new_price, Quantity new_quantity);
    bool cancel_order(OrderId id);

    [[nodiscard]] const OrderBook& order_book() const;
    [[nodiscard]] std::optional<OrderState> order_state(OrderId id) const;

private:
    static void record_state_update(const OrderState& state, EngineResult& result);
    void refresh_order_state(OrderId id, EngineResult& result);

    OrderBook order_book_;
    std::unordered_map<OrderId, OrderState> order_states_;
};

[[nodiscard]] std::string to_string(OrderStatus status);

}  // namespace ob

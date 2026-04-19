#include "matching_engine.h"

namespace ob {

std::string to_string(OrderStatus status) {
    switch (status) {
        case OrderStatus::Active:
            return "ACTIVE";
        case OrderStatus::Filled:
            return "FILLED";
        case OrderStatus::Cancelled:
            return "CANCELLED";
        case OrderStatus::Replaced:
            return "REPLACED";
        case OrderStatus::Rejected:
            return "REJECTED";
    }
    return "UNKNOWN";
}

void MatchingEngine::record_state_update(const OrderState& state, EngineResult& result) {
    for (auto& existing : result.state_updates) {
        if (existing.order_id == state.order_id) {
            existing = state;
            return;
        }
    }
    result.state_updates.push_back(state);
}

EngineResult MatchingEngine::submit_limit_order(OrderId id, Side side, Price price, Quantity quantity) {
    EngineResult result;
    try {
        result.summary = order_book_.add_limit_order(id, side, price, quantity, result.trades);
        refresh_order_state(id, result);
        for (const auto& trade : result.trades) {
            refresh_order_state(trade.buy_order_id, result);
            refresh_order_state(trade.sell_order_id, result);
        }
    } catch (...) {
        order_states_[id] = OrderState {.order_id = id, .status = OrderStatus::Rejected, .remaining_quantity = std::nullopt};
        record_state_update(order_states_.at(id), result);
        throw;
    }
    return result;
}

EngineResult MatchingEngine::submit_market_order(OrderId id, Side side, Quantity quantity) {
    EngineResult result;
    try {
        result.summary = order_book_.add_market_order(id, side, quantity, result.trades);
        refresh_order_state(id, result);
        for (const auto& trade : result.trades) {
            refresh_order_state(trade.buy_order_id, result);
            refresh_order_state(trade.sell_order_id, result);
        }
    } catch (...) {
        order_states_[id] = OrderState {.order_id = id, .status = OrderStatus::Rejected, .remaining_quantity = std::nullopt};
        record_state_update(order_states_.at(id), result);
        throw;
    }
    return result;
}

EngineResult MatchingEngine::replace_order(OrderId id, Price new_price, Quantity new_quantity) {
    EngineResult result;
    const auto modify_result = order_book_.modify_order(id, new_price, new_quantity, result.trades);
    if (!modify_result.has_value()) {
        result.summary = AddResult {.requested_quantity = new_quantity};
        order_states_[id] = OrderState {.order_id = id, .status = OrderStatus::Rejected, .remaining_quantity = std::nullopt};
        record_state_update(order_states_.at(id), result);
        return result;
    }
    result.summary = *modify_result;
    if (new_quantity == 0) {
        order_states_[id] = OrderState {.order_id = id, .status = OrderStatus::Cancelled, .remaining_quantity = std::nullopt};
        record_state_update(order_states_.at(id), result);
        return result;
    }

    refresh_order_state(id, result);
    for (const auto& trade : result.trades) {
        refresh_order_state(trade.buy_order_id, result);
        refresh_order_state(trade.sell_order_id, result);
    }
    return result;
}

bool MatchingEngine::cancel_order(OrderId id) {
    const bool removed = order_book_.cancel_order(id);
    if (removed) {
        order_states_[id] = OrderState {.order_id = id, .status = OrderStatus::Cancelled, .remaining_quantity = std::nullopt};
    }
    return removed;
}

const OrderBook& MatchingEngine::order_book() const {
    return order_book_;
}

std::optional<OrderState> MatchingEngine::order_state(OrderId id) const {
    const auto it = order_states_.find(id);
    if (it == order_states_.end()) {
        return std::nullopt;
    }
    return it->second;
}

void MatchingEngine::refresh_order_state(OrderId id, EngineResult& result) {
    OrderState state {.order_id = id};
    const auto remaining = order_book_.remaining_quantity(id);
    if (remaining.has_value()) {
        state.status = OrderStatus::Active;
        state.remaining_quantity = remaining;
    } else {
        const auto existing = order_states_.find(id);
        if (existing != order_states_.end() && existing->second.status == OrderStatus::Cancelled) {
            state = existing->second;
        } else {
            state.status = OrderStatus::Filled;
            state.remaining_quantity = 0;
        }
    }

    order_states_[id] = state;
    record_state_update(state, result);
}

}  // namespace ob

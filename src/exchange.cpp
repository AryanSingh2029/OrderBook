#include "exchange.h"

#include <algorithm>
#include <exception>

namespace ob {

RoutedResult Exchange::submit_limit_order(
    const std::string& symbol,
    OrderId id,
    Side side,
    Price price,
    Quantity quantity) {
    RoutedResult result;
    try {
        auto& engine = books_[symbol];
        result.engine_result = engine.submit_limit_order(id, side, price, quantity);
        if (result.engine_result.summary.inserted_into_book) {
            order_to_symbol_[id] = symbol;
        }
        for (const auto& update : result.engine_result.state_updates) {
            if (update.status == OrderStatus::Filled || update.status == OrderStatus::Cancelled) {
                order_to_symbol_.erase(update.order_id);
            }
        }
    } catch (const std::exception& ex) {
        result.rejected = true;
        result.rejection_message = ex.what();
    }
    return result;
}

RoutedResult Exchange::submit_market_order(
    const std::string& symbol,
    OrderId id,
    Side side,
    Quantity quantity) {
    RoutedResult result;
    try {
        auto& engine = books_[symbol];
        result.engine_result = engine.submit_market_order(id, side, quantity);
        order_to_symbol_.erase(id);
        for (const auto& update : result.engine_result.state_updates) {
            if (update.status == OrderStatus::Filled || update.status == OrderStatus::Cancelled) {
                order_to_symbol_.erase(update.order_id);
            }
        }
    } catch (const std::exception& ex) {
        result.rejected = true;
        result.rejection_message = ex.what();
    }
    return result;
}

RoutedResult Exchange::cancel_order(OrderId id) {
    RoutedResult result;
    const auto symbol_it = order_to_symbol_.find(id);
    if (symbol_it == order_to_symbol_.end()) {
        return result;
    }

    auto book_it = books_.find(symbol_it->second);
    if (book_it == books_.end()) {
        order_to_symbol_.erase(symbol_it);
        return result;
    }

    result.cancel_succeeded = book_it->second.cancel_order(id);
    if (result.cancel_succeeded) {
        order_to_symbol_.erase(symbol_it);
    }
    return result;
}

const MatchingEngine* Exchange::engine_for(const std::string& symbol) const {
    const auto it = books_.find(symbol);
    if (it == books_.end()) {
        return nullptr;
    }
    return &it->second;
}

std::vector<std::string> Exchange::symbols() const {
    std::vector<std::string> result;
    result.reserve(books_.size());
    for (const auto& [symbol, engine] : books_) {
        (void)engine;
        result.push_back(symbol);
    }
    std::sort(result.begin(), result.end());
    return result;
}

std::size_t Exchange::total_resting_orders() const {
    std::size_t total = 0;
    for (const auto& [symbol, engine] : books_) {
        (void)symbol;
        total += engine.order_book().order_count();
    }
    return total;
}

}  // namespace ob

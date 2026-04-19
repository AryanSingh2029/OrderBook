#pragma once

#include <cstdint>
#include <string>

namespace ob {

enum class Side {
    Buy,
    Sell
};

enum class OrderType {
    Limit,
    Market
};

using OrderId = std::uint64_t;
using Price = std::int64_t;
using Quantity = std::uint64_t;
using Timestamp = std::uint64_t;

struct Order {
    OrderId order_id {};
    Side side {Side::Buy};
    OrderType type {OrderType::Limit};
    Price price {};
    Quantity quantity {};
    Quantity remaining_quantity {};
    Timestamp timestamp {};
};

struct Trade {
    OrderId buy_order_id {};
    OrderId sell_order_id {};
    Price trade_price {};
    Quantity trade_quantity {};
    Timestamp timestamp {};
};

struct AddResult {
    Quantity requested_quantity {};
    Quantity executed_quantity {};
    Quantity remaining_quantity {};
    bool inserted_into_book {};
};

[[nodiscard]] std::string to_string(Side side);
[[nodiscard]] std::string to_string(OrderType type);

}  // namespace ob

#include "order.h"

namespace ob {

std::string to_string(Side side) {
    return side == Side::Buy ? "BUY" : "SELL";
}

std::string to_string(OrderType type) {
    return type == OrderType::Limit ? "LIMIT" : "MARKET";
}

}  // namespace ob

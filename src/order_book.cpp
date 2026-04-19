#include "order_book.h"

#include <algorithm>
#include <stdexcept>

namespace ob {

namespace {

template <typename Book>
LevelSnapshot make_snapshot(typename Book::const_iterator it) {
    return LevelSnapshot {
        .price = it->second.price,
        .quantity = it->second.total_quantity,
        .order_count = it->second.order_count,
    };
}

bool crosses(Side side, OrderType type, Price incoming_price, Price resting_price) {
    if (type == OrderType::Market) {
        return true;
    }
    if (side == Side::Buy) {
        return incoming_price >= resting_price;
    }
    return incoming_price <= resting_price;
}

}  // namespace

AddResult OrderBook::add_limit_order(OrderId id, Side side, Price price, Quantity quantity, std::vector<Trade>& trades) {
    return add_order(id, side, OrderType::Limit, price, quantity, trades);
}

AddResult OrderBook::add_market_order(OrderId id, Side side, Quantity quantity, std::vector<Trade>& trades) {
    return add_order(id, side, OrderType::Market, 0, quantity, trades);
}

AddResult OrderBook::add_order(
    OrderId id,
    Side side,
    OrderType type,
    Price price,
    Quantity quantity,
    std::vector<Trade>& trades) {
    if (quantity == 0) {
        throw std::invalid_argument("quantity must be positive");
    }
    if (type == OrderType::Limit && price <= 0) {
        throw std::invalid_argument("limit price must be positive");
    }
    if (order_index_.contains(id)) {
        throw std::invalid_argument("duplicate order id");
    }

    const Quantity requested = quantity;
    Quantity remaining = quantity;
    const Timestamp incoming_timestamp = next_timestamp();

    auto match_against = [&](auto& opposite_book) {
        while (remaining > 0 && !opposite_book.empty()) {
            auto best_level_it = opposite_book.begin();
            if (!crosses(side, type, price, best_level_it->first)) {
                break;
            }

            auto& level = best_level_it->second;
            auto resting_slot = level.head;
            while (resting_slot != npos && remaining > 0) {
                auto& node = order_nodes_[resting_slot];
                auto& resting_order = node.order;
                const Quantity executed = std::min(remaining, resting_order.remaining_quantity);
                trades.push_back(Trade {
                    .buy_order_id = side == Side::Buy ? id : resting_order.order_id,
                    .sell_order_id = side == Side::Sell ? id : resting_order.order_id,
                    .trade_price = resting_order.price,
                    .trade_quantity = executed,
                    .timestamp = next_timestamp(),
                });

                remaining -= executed;
                resting_order.remaining_quantity -= executed;
                level.total_quantity -= executed;

                if (resting_order.remaining_quantity == 0) {
                    const auto next_slot = node.next;
                    order_index_.erase(resting_order.order_id);
                    erase_order(OrderHandle {.side = resting_order.side, .price = resting_order.price, .slot = resting_slot});
                    resting_slot = next_slot;
                } else {
                    resting_slot = node.next;
                }
            }
        }
    };

    if (side == Side::Buy) {
        match_against(asks_);
    } else {
        match_against(bids_);
    }

    bool inserted = false;
    if (type == OrderType::Limit && remaining > 0) {
        insert_resting_order(id, side, price, remaining, incoming_timestamp);
        inserted = true;
    }

    return AddResult {
        .requested_quantity = requested,
        .executed_quantity = requested - remaining,
        .remaining_quantity = remaining,
        .inserted_into_book = inserted,
    };
}

bool OrderBook::cancel_order(OrderId id) {
    const auto it = order_index_.find(id);
    if (it == order_index_.end()) {
        return false;
    }

    erase_order(it->second);
    order_index_.erase(it);
    return true;
}

std::optional<AddResult> OrderBook::modify_order(OrderId id, Price new_price, Quantity new_quantity, std::vector<Trade>& trades) {
    const auto it = order_index_.find(id);
    if (it == order_index_.end()) {
        return std::nullopt;
    }
    if (new_quantity == 0) {
        erase_order(it->second);
        order_index_.erase(it);
        return AddResult {
            .requested_quantity = 0,
            .executed_quantity = 0,
            .remaining_quantity = 0,
            .inserted_into_book = false,
        };
    }
    if (new_price <= 0) {
        throw std::invalid_argument("limit price must be positive");
    }

    const Side side = it->second.side;
    erase_order(it->second);
    order_index_.erase(it);
    return add_limit_order(id, side, new_price, new_quantity, trades);
}

TopOfBook OrderBook::top_of_book() const {
    return TopOfBook {
        .best_bid = snapshot_best_level(bids_),
        .best_ask = snapshot_best_level(asks_),
    };
}

std::optional<double> OrderBook::mid_price() const {
    const auto top = top_of_book();
    if (!top.best_bid || !top.best_ask) {
        return std::nullopt;
    }
    return (static_cast<double>(top.best_bid->price) + static_cast<double>(top.best_ask->price)) / 2.0;
}

std::optional<Price> OrderBook::spread() const {
    const auto top = top_of_book();
    if (!top.best_bid || !top.best_ask) {
        return std::nullopt;
    }
    return top.best_ask->price - top.best_bid->price;
}

Quantity OrderBook::total_volume(Side side) const {
    return side == Side::Buy ? book_volume(bids_) : book_volume(asks_);
}

std::optional<Quantity> OrderBook::volume_at_price(Side side, Price price) const {
    return side == Side::Buy ? lookup_volume(bids_, price) : lookup_volume(asks_, price);
}

std::optional<Quantity> OrderBook::remaining_quantity(OrderId id) const {
    const auto it = order_index_.find(id);
    if (it == order_index_.end()) {
        return std::nullopt;
    }
    return order_nodes_[it->second.slot].order.remaining_quantity;
}

std::optional<OrderMetadata> OrderBook::order_metadata(OrderId id) const {
    const auto it = order_index_.find(id);
    if (it == order_index_.end()) {
        return std::nullopt;
    }

    const auto& order = order_nodes_[it->second.slot].order;
    return OrderMetadata {
        .order_id = order.order_id,
        .side = order.side,
        .type = order.type,
        .price = order.price,
        .quantity = order.quantity,
        .remaining_quantity = order.remaining_quantity,
        .timestamp = order.timestamp,
    };
}

std::optional<QueuePosition> OrderBook::queue_position(OrderId id) const {
    const auto handle_it = order_index_.find(id);
    if (handle_it == order_index_.end()) {
        return std::nullopt;
    }

    const auto& handle = handle_it->second;
    const auto* level = [&]() -> const PriceLevel* {
        if (handle.side == Side::Buy) {
            const auto it = bids_.find(handle.price);
            return it == bids_.end() ? nullptr : &it->second;
        }
        const auto it = asks_.find(handle.price);
        return it == asks_.end() ? nullptr : &it->second;
    }();

    if (level == nullptr) {
        return std::nullopt;
    }

    QueuePosition result {
        .orders_ahead = 0,
        .quantity_ahead = 0,
        .level_order_count = level->order_count,
        .level_total_quantity = level->total_quantity,
    };

    for (auto slot = level->head; slot != npos; slot = order_nodes_[slot].next) {
        if (slot == handle.slot) {
            return result;
        }
        ++result.orders_ahead;
        result.quantity_ahead += order_nodes_[slot].order.remaining_quantity;
    }

    return std::nullopt;
}

bool OrderBook::contains_order(OrderId id) const {
    return order_index_.contains(id);
}

std::vector<LevelSnapshot> OrderBook::levels(Side side, std::size_t depth) const {
    return side == Side::Buy ? snapshot_levels(bids_, depth) : snapshot_levels(asks_, depth);
}

std::size_t OrderBook::order_count() const {
    return order_index_.size();
}

template <typename Book>
std::optional<LevelSnapshot> OrderBook::snapshot_best_level(const Book& book) {
    if (book.empty()) {
        return std::nullopt;
    }
    return make_snapshot<Book>(book.begin());
}

template <typename Book>
std::vector<LevelSnapshot> OrderBook::snapshot_levels(const Book& book, std::size_t depth) {
    std::vector<LevelSnapshot> snapshots;
    snapshots.reserve(depth);

    auto it = book.begin();
    for (std::size_t i = 0; i < depth && it != book.end(); ++i, ++it) {
        snapshots.push_back(make_snapshot<Book>(it));
    }
    return snapshots;
}

template <typename Book>
std::optional<Quantity> OrderBook::lookup_volume(const Book& book, Price price) {
    const auto it = book.find(price);
    if (it == book.end()) {
        return std::nullopt;
    }
    return it->second.total_quantity;
}

template <typename Book>
Quantity OrderBook::book_volume(const Book& book) {
    Quantity total = 0;
    for (const auto& [price, level] : book) {
        (void)price;
        total += level.total_quantity;
    }
    return total;
}

void OrderBook::insert_resting_order(OrderId id, Side side, Price price, Quantity quantity, Timestamp timestamp) {
    const auto slot = allocate_slot();
    auto& node = order_nodes_[slot];
    node.order = Order {
        .order_id = id,
        .side = side,
        .type = OrderType::Limit,
        .price = price,
        .quantity = quantity,
        .remaining_quantity = quantity,
        .timestamp = timestamp,
    };
    node.prev = npos;
    node.next = npos;
    node.active = true;

    if (side == Side::Buy) {
        auto& level = bids_[price];
        level.price = price;
        level.total_quantity += quantity;
        ++level.order_count;
        if (level.tail == npos) {
            level.head = slot;
            level.tail = slot;
        } else {
            order_nodes_[level.tail].next = slot;
            node.prev = level.tail;
            level.tail = slot;
        }
    } else {
        auto& level = asks_[price];
        level.price = price;
        level.total_quantity += quantity;
        ++level.order_count;
        if (level.tail == npos) {
            level.head = slot;
            level.tail = slot;
        } else {
            order_nodes_[level.tail].next = slot;
            node.prev = level.tail;
            level.tail = slot;
        }
    }

    order_index_.emplace(id, OrderHandle {.side = side, .price = price, .slot = slot});
}

void OrderBook::erase_order(const OrderHandle& handle) {
    if (handle.side == Side::Buy) {
        auto level_it = bids_.find(handle.price);
        if (level_it == bids_.end()) {
            return;
        }
        auto& level = level_it->second;
        auto& node = order_nodes_[handle.slot];
        if (node.prev != npos) {
            order_nodes_[node.prev].next = node.next;
        } else {
            level.head = node.next;
        }
        if (node.next != npos) {
            order_nodes_[node.next].prev = node.prev;
        } else {
            level.tail = node.prev;
        }
        if (level.total_quantity >= node.order.remaining_quantity) {
            level.total_quantity -= node.order.remaining_quantity;
        }
        if (level.order_count > 0) {
            --level.order_count;
        }
        free_slot(handle.slot);
        if (level.order_count == 0) {
            bids_.erase(level_it);
        }
        return;
    }

    auto level_it = asks_.find(handle.price);
    if (level_it == asks_.end()) {
        return;
    }
    auto& level = level_it->second;
    auto& node = order_nodes_[handle.slot];
    if (node.prev != npos) {
        order_nodes_[node.prev].next = node.next;
    } else {
        level.head = node.next;
    }
    if (node.next != npos) {
        order_nodes_[node.next].prev = node.prev;
    } else {
        level.tail = node.prev;
    }
    if (level.total_quantity >= node.order.remaining_quantity) {
        level.total_quantity -= node.order.remaining_quantity;
    }
    if (level.order_count > 0) {
        --level.order_count;
    }
    free_slot(handle.slot);
    if (level.order_count == 0) {
        asks_.erase(level_it);
    }
}

std::size_t OrderBook::allocate_slot() {
    if (!free_slots_.empty()) {
        const auto slot = free_slots_.back();
        free_slots_.pop_back();
        return slot;
    }

    order_nodes_.push_back(OrderNode {});
    return order_nodes_.size() - 1;
}

void OrderBook::free_slot(std::size_t slot) {
    auto& node = order_nodes_[slot];
    node.active = false;
    node.prev = npos;
    node.next = npos;
    node.order = Order {};
    free_slots_.push_back(slot);
}

Timestamp OrderBook::next_timestamp() {
    return next_timestamp_++;
}

template std::optional<LevelSnapshot> OrderBook::snapshot_best_level(const BidBook& book);
template std::optional<LevelSnapshot> OrderBook::snapshot_best_level(const AskBook& book);
template std::vector<LevelSnapshot> OrderBook::snapshot_levels(const BidBook& book, std::size_t depth);
template std::vector<LevelSnapshot> OrderBook::snapshot_levels(const AskBook& book, std::size_t depth);
template std::optional<Quantity> OrderBook::lookup_volume(const BidBook& book, Price price);
template std::optional<Quantity> OrderBook::lookup_volume(const AskBook& book, Price price);
template Quantity OrderBook::book_volume(const BidBook& book);
template Quantity OrderBook::book_volume(const AskBook& book);

}  // namespace ob

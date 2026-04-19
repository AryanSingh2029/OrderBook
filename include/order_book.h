#pragma once

#include "order.h"

#include <functional>
#include <limits>
#include <map>
#include <optional>
#include <unordered_map>
#include <vector>

namespace ob {

struct LevelSnapshot {
    Price price {};
    Quantity quantity {};
    std::size_t order_count {};
};

struct PriceLevel {
    Price price {};
    Quantity total_quantity {};
    std::size_t order_count {};
    std::size_t head {std::numeric_limits<std::size_t>::max()};
    std::size_t tail {std::numeric_limits<std::size_t>::max()};
};

struct TopOfBook {
    std::optional<LevelSnapshot> best_bid;
    std::optional<LevelSnapshot> best_ask;
};

struct OrderMetadata {
    OrderId order_id {};
    Side side {Side::Buy};
    OrderType type {OrderType::Limit};
    Price price {};
    Quantity quantity {};
    Quantity remaining_quantity {};
    Timestamp timestamp {};
};

struct QueuePosition {
    std::size_t orders_ahead {};
    Quantity quantity_ahead {};
    std::size_t level_order_count {};
    Quantity level_total_quantity {};
};

class OrderBook {
public:
    AddResult add_limit_order(OrderId id, Side side, Price price, Quantity quantity, std::vector<Trade>& trades);
    AddResult add_market_order(OrderId id, Side side, Quantity quantity, std::vector<Trade>& trades);
    bool cancel_order(OrderId id);
    std::optional<AddResult> modify_order(OrderId id, Price new_price, Quantity new_quantity, std::vector<Trade>& trades);

    [[nodiscard]] TopOfBook top_of_book() const;
    [[nodiscard]] std::optional<double> mid_price() const;
    [[nodiscard]] std::optional<Price> spread() const;
    [[nodiscard]] Quantity total_volume(Side side) const;
    [[nodiscard]] std::optional<Quantity> volume_at_price(Side side, Price price) const;
    [[nodiscard]] std::optional<Quantity> remaining_quantity(OrderId id) const;
    [[nodiscard]] std::optional<OrderMetadata> order_metadata(OrderId id) const;
    [[nodiscard]] std::optional<QueuePosition> queue_position(OrderId id) const;
    [[nodiscard]] bool contains_order(OrderId id) const;
    [[nodiscard]] std::vector<LevelSnapshot> levels(Side side, std::size_t depth) const;
    [[nodiscard]] std::size_t order_count() const;

private:
    using BidBook = std::map<Price, PriceLevel, std::greater<>>;
    using AskBook = std::map<Price, PriceLevel, std::less<>>;

    static constexpr std::size_t npos = std::numeric_limits<std::size_t>::max();

    struct OrderNode {
        Order order;
        std::size_t prev {npos};
        std::size_t next {npos};
        bool active {false};
    };

    struct OrderHandle {
        Side side {Side::Buy};
        Price price {};
        std::size_t slot {npos};
    };

    AddResult add_order(OrderId id, Side side, OrderType type, Price price, Quantity quantity, std::vector<Trade>& trades);

    template <typename Book>
    static std::optional<LevelSnapshot> snapshot_best_level(const Book& book);

    template <typename Book>
    static std::vector<LevelSnapshot> snapshot_levels(const Book& book, std::size_t depth);

    template <typename Book>
    static std::optional<Quantity> lookup_volume(const Book& book, Price price);

    template <typename Book>
    static Quantity book_volume(const Book& book);

    void insert_resting_order(OrderId id, Side side, Price price, Quantity quantity, Timestamp timestamp);
    void erase_order(const OrderHandle& handle);
    [[nodiscard]] std::size_t allocate_slot();
    void free_slot(std::size_t slot);
    [[nodiscard]] Timestamp next_timestamp();

    BidBook bids_;
    AskBook asks_;
    std::unordered_map<OrderId, OrderHandle> order_index_;
    std::vector<OrderNode> order_nodes_;
    std::vector<std::size_t> free_slots_;
    Timestamp next_timestamp_ {1};
};

}  // namespace ob

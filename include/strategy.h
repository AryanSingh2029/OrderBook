#pragma once

#include "simulator.h"

#include <optional>
#include <vector>

namespace ob {

struct StrategyConfig {
    double imbalance_threshold {0.50};
    Quantity order_quantity {10};
};

struct StrategyFill {
    Timestamp timestamp {};
    Side side {Side::Buy};
    Price price {};
    Quantity quantity {};
    std::optional<double> reference_mid_price;
};

struct StrategyStats {
    std::size_t external_events_processed {};
    std::size_t signals_triggered {};
    std::size_t orders_sent {};
    std::size_t filled_orders {};
    Quantity requested_quantity {};
    Quantity filled_quantity {};
    std::int64_t inventory {};
    double average_cost {};
    double realized_pnl {};
    std::optional<double> unrealized_pnl;
    std::optional<double> total_pnl;
    double fill_ratio {};
    std::size_t adverse_selection_events {};
    double adverse_selection_notional {};
};

struct StrategyRunResult {
    StrategyStats stats;
    std::vector<StrategyFill> fills;
    TopOfBook final_top_of_book;
};

class ImbalanceTakerStrategy {
public:
    explicit ImbalanceTakerStrategy(StrategyConfig config = {});

    StrategyRunResult run(const std::vector<Event>& events) const;

private:
    struct PendingMarkout {
        Side side {Side::Buy};
        Price fill_price {};
        Quantity quantity {};
    };

    static void apply_fill_to_inventory(StrategyStats& stats, Side side, Price price, Quantity quantity);
    static void resolve_pending_markouts(
        StrategyStats& stats,
        std::vector<PendingMarkout>& pending_markouts,
        const TopOfBook& top_of_book);

    StrategyConfig config_;
};

}  // namespace ob

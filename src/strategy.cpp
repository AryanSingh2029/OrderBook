#include "strategy.h"

#include "analytics.h"

#include <cmath>

namespace ob {

ImbalanceTakerStrategy::ImbalanceTakerStrategy(StrategyConfig config) : config_(config) {}

StrategyRunResult ImbalanceTakerStrategy::run(const std::vector<Event>& events) const {
    MatchingEngine engine;
    StrategyRunResult result;
    std::vector<PendingMarkout> pending_markouts;
    OrderId next_strategy_order_id = 1'000'000'000ULL;

    for (const auto& event : events) {
        switch (event.type) {
            case EventType::Add:
                (void)engine.submit_limit_order(event.order_id, event.side, event.price, event.quantity);
                break;
            case EventType::Market:
                (void)engine.submit_market_order(event.order_id, event.side, event.quantity);
                break;
            case EventType::Cancel:
                (void)engine.cancel_order(event.order_id);
                break;
        }

        ++result.stats.external_events_processed;
        const auto top = engine.order_book().top_of_book();
        resolve_pending_markouts(result.stats, pending_markouts, top);

        const auto imbalance = Analytics::compute_imbalance(engine.order_book(), 5).top_of_book_imbalance;
        if (!imbalance.has_value()) {
            continue;
        }

        std::optional<Side> signal_side;
        if (*imbalance >= config_.imbalance_threshold && top.best_ask.has_value()) {
            signal_side = Side::Buy;
        } else if (*imbalance <= -config_.imbalance_threshold && top.best_bid.has_value()) {
            signal_side = Side::Sell;
        }

        if (!signal_side.has_value()) {
            continue;
        }

        ++result.stats.signals_triggered;
        ++result.stats.orders_sent;
        result.stats.requested_quantity += config_.order_quantity;

        const auto signal_mid = engine.order_book().mid_price();
        const auto strategy_result =
            engine.submit_market_order(next_strategy_order_id++, *signal_side, config_.order_quantity);

        if (strategy_result.summary.executed_quantity > 0) {
            ++result.stats.filled_orders;
        }
        result.stats.filled_quantity += strategy_result.summary.executed_quantity;

        for (const auto& trade : strategy_result.trades) {
            const Quantity qty = trade.trade_quantity;
            const Price px = trade.trade_price;
            apply_fill_to_inventory(result.stats, *signal_side, px, qty);

            result.fills.push_back(StrategyFill {
                .timestamp = trade.timestamp,
                .side = *signal_side,
                .price = px,
                .quantity = qty,
                .reference_mid_price = signal_mid,
            });
            pending_markouts.push_back(PendingMarkout {
                .side = *signal_side,
                .fill_price = px,
                .quantity = qty,
            });
        }
    }

    const auto final_top = engine.order_book().top_of_book();
    result.final_top_of_book = final_top;
    resolve_pending_markouts(result.stats, pending_markouts, final_top);

    if (result.stats.requested_quantity > 0) {
        result.stats.fill_ratio =
            static_cast<double>(result.stats.filled_quantity) / static_cast<double>(result.stats.requested_quantity);
    }

    if (const auto final_mid = engine.order_book().mid_price()) {
        if (result.stats.inventory > 0) {
            result.stats.unrealized_pnl =
                (*final_mid - result.stats.average_cost) * static_cast<double>(result.stats.inventory);
        } else if (result.stats.inventory < 0) {
            result.stats.unrealized_pnl =
                (result.stats.average_cost - *final_mid) * static_cast<double>(-result.stats.inventory);
        } else {
            result.stats.unrealized_pnl = 0.0;
        }
        result.stats.total_pnl = result.stats.realized_pnl + *result.stats.unrealized_pnl;
    } else if (result.stats.inventory == 0) {
        result.stats.unrealized_pnl = 0.0;
        result.stats.total_pnl = result.stats.realized_pnl;
    }

    return result;
}

void ImbalanceTakerStrategy::apply_fill_to_inventory(StrategyStats& stats, Side side, Price price, Quantity quantity) {
    const auto qty = static_cast<std::int64_t>(quantity);
    const double px = static_cast<double>(price);

    if (side == Side::Buy) {
        if (stats.inventory >= 0) {
            const auto existing = static_cast<double>(stats.inventory);
            const auto incoming = static_cast<double>(qty);
            const auto total = existing + incoming;
            stats.average_cost = total > 0.0 ? ((stats.average_cost * existing) + (px * incoming)) / total : 0.0;
            stats.inventory += qty;
            return;
        }

        const auto covering = std::min<std::int64_t>(qty, -stats.inventory);
        stats.realized_pnl += (stats.average_cost - px) * static_cast<double>(covering);
        stats.inventory += covering;

        if (stats.inventory == 0) {
            stats.average_cost = 0.0;
        }

        const auto remaining = qty - covering;
        if (remaining > 0) {
            stats.inventory = remaining;
            stats.average_cost = px;
        }
        return;
    }

    if (stats.inventory <= 0) {
        const auto existing = static_cast<double>(-stats.inventory);
        const auto incoming = static_cast<double>(qty);
        const auto total = existing + incoming;
        stats.average_cost = total > 0.0 ? ((stats.average_cost * existing) + (px * incoming)) / total : 0.0;
        stats.inventory -= qty;
        return;
    }

    const auto closing = std::min<std::int64_t>(qty, stats.inventory);
    stats.realized_pnl += (px - stats.average_cost) * static_cast<double>(closing);
    stats.inventory -= closing;

    if (stats.inventory == 0) {
        stats.average_cost = 0.0;
    }

    const auto remaining = qty - closing;
    if (remaining > 0) {
        stats.inventory = -remaining;
        stats.average_cost = px;
    }
}

void ImbalanceTakerStrategy::resolve_pending_markouts(
    StrategyStats& stats,
    std::vector<PendingMarkout>& pending_markouts,
    const TopOfBook& top_of_book) {
    if (!top_of_book.best_bid.has_value() || !top_of_book.best_ask.has_value()) {
        pending_markouts.clear();
        return;
    }

    const double mid =
        (static_cast<double>(top_of_book.best_bid->price) + static_cast<double>(top_of_book.best_ask->price)) / 2.0;

    for (const auto& pending : pending_markouts) {
        const double fill_price = static_cast<double>(pending.fill_price);
        const double qty = static_cast<double>(pending.quantity);
        if (pending.side == Side::Buy && mid < fill_price) {
            ++stats.adverse_selection_events;
            stats.adverse_selection_notional += (fill_price - mid) * qty;
        } else if (pending.side == Side::Sell && mid > fill_price) {
            ++stats.adverse_selection_events;
            stats.adverse_selection_notional += (mid - fill_price) * qty;
        }
    }

    pending_markouts.clear();
}

}  // namespace ob

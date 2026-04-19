#include "analytics.h"
#include "simulator.h"

#include <numeric>

namespace ob {

namespace {

std::optional<double> imbalance_ratio(Quantity left, Quantity right) {
    const Quantity total = left + right;
    if (total == 0) {
        return std::nullopt;
    }
    return (static_cast<double>(left) - static_cast<double>(right)) / static_cast<double>(total);
}

std::optional<Price> top_price_for_event(const Event& event, const TopOfBook& top) {
    if (event.side == Side::Buy) {
        return top.best_ask ? std::optional<Price>(top.best_ask->price) : std::nullopt;
    }
    return top.best_bid ? std::optional<Price>(top.best_bid->price) : std::nullopt;
}

Quantity top_quantity_for_event(const Event& event, const TopOfBook& top) {
    if (event.side == Side::Buy) {
        return top.best_ask ? top.best_ask->quantity : 0;
    }
    return top.best_bid ? top.best_bid->quantity : 0;
}

}  // namespace

TopOfBookMetrics Analytics::compute_top_of_book(const OrderBook& book) {
    const auto top = book.top_of_book();
    return TopOfBookMetrics {
        .best_bid = top.best_bid ? std::optional<Price>(top.best_bid->price) : std::nullopt,
        .best_ask = top.best_ask ? std::optional<Price>(top.best_ask->price) : std::nullopt,
        .spread = book.spread(),
        .mid_price = book.mid_price(),
    };
}

DepthMetrics Analytics::compute_depth(const OrderBook& book, std::size_t depth) {
    const auto bids = book.levels(Side::Buy, depth);
    const auto asks = book.levels(Side::Sell, depth);

    DepthMetrics metrics;
    metrics.bid_cumulative_depth.reserve(bids.size());
    metrics.ask_cumulative_depth.reserve(asks.size());

    Quantity bid_running = 0;
    for (const auto& level : bids) {
        bid_running += level.quantity;
        metrics.bid_cumulative_depth.push_back(bid_running);
    }
    metrics.bid_top_n_volume = bid_running;

    Quantity ask_running = 0;
    for (const auto& level : asks) {
        ask_running += level.quantity;
        metrics.ask_cumulative_depth.push_back(ask_running);
    }
    metrics.ask_top_n_volume = ask_running;

    return metrics;
}

ImbalanceMetrics Analytics::compute_imbalance(const OrderBook& book, std::size_t depth) {
    const auto top = book.top_of_book();
    const auto depth_metrics = compute_depth(book, depth);

    const Quantity total_bid = book.total_volume(Side::Buy);
    const Quantity total_ask = book.total_volume(Side::Sell);

    return ImbalanceMetrics {
        .total_bid_volume = total_bid,
        .total_ask_volume = total_ask,
        .top_of_book_imbalance = imbalance_ratio(
            top.best_bid ? top.best_bid->quantity : 0,
            top.best_ask ? top.best_ask->quantity : 0),
        .depth_imbalance = imbalance_ratio(depth_metrics.bid_top_n_volume, depth_metrics.ask_top_n_volume),
    };
}

TradeMetrics Analytics::compute_trade_metrics(const std::vector<Trade>& trades) {
    TradeMetrics metrics;
    metrics.execution_count = trades.size();

    if (trades.empty()) {
        return metrics;
    }

    double notional = 0.0;
    for (const auto& trade : trades) {
        metrics.traded_volume += trade.trade_quantity;
        notional += static_cast<double>(trade.trade_price) * static_cast<double>(trade.trade_quantity);
    }

    metrics.vwap = notional / static_cast<double>(metrics.traded_volume);
    metrics.average_trade_size = static_cast<double>(metrics.traded_volume) / static_cast<double>(metrics.execution_count);
    return metrics;
}

ExecutionMetrics Analytics::compute_execution_metrics(
    const Event& event,
    const EngineResult& result,
    const TopOfBook& before_top,
    const TopOfBook& after_top) {
    ExecutionMetrics metrics;
    if (result.summary.requested_quantity > 0) {
        metrics.fill_ratio =
            static_cast<double>(result.summary.executed_quantity) / static_cast<double>(result.summary.requested_quantity);
    }

    const auto initial_reference_price = top_price_for_event(event, before_top);
    const auto trade_metrics = compute_trade_metrics(result.trades);
    if (initial_reference_price.has_value() && trade_metrics.vwap.has_value()) {
        if (event.side == Side::Buy) {
            metrics.slippage = *trade_metrics.vwap - static_cast<double>(*initial_reference_price);
        } else {
            metrics.slippage = static_cast<double>(*initial_reference_price) - *trade_metrics.vwap;
        }
    }

    const Quantity before_top_qty = top_quantity_for_event(event, before_top);
    const auto before_top_price = top_price_for_event(event, before_top);
    const auto after_top_price = top_price_for_event(event, after_top);
    if (before_top_price.has_value() && before_top_price != after_top_price) {
        metrics.queue_depletion = before_top_qty;
    } else {
        Quantity after_top_qty = 0;
        if (event.side == Side::Buy) {
            after_top_qty = after_top.best_ask ? after_top.best_ask->quantity : 0;
        } else {
            after_top_qty = after_top.best_bid ? after_top.best_bid->quantity : 0;
        }
        metrics.queue_depletion = before_top_qty >= after_top_qty ? before_top_qty - after_top_qty : 0;
    }

    return metrics;
}

MarketMetrics Analytics::compute_market_metrics(const OrderBook& book, std::size_t depth) {
    return MarketMetrics {
        .top_of_book = compute_top_of_book(book),
        .depth = compute_depth(book, depth),
        .imbalance = compute_imbalance(book, depth),
    };
}

}  // namespace ob

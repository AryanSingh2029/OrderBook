#pragma once

#include "matching_engine.h"

#include <optional>
#include <vector>

namespace ob {

struct Event;

struct TopOfBookMetrics {
    std::optional<Price> best_bid;
    std::optional<Price> best_ask;
    std::optional<Price> spread;
    std::optional<double> mid_price;
};

struct DepthMetrics {
    Quantity bid_top_n_volume {};
    Quantity ask_top_n_volume {};
    std::vector<Quantity> bid_cumulative_depth;
    std::vector<Quantity> ask_cumulative_depth;
};

struct ImbalanceMetrics {
    Quantity total_bid_volume {};
    Quantity total_ask_volume {};
    std::optional<double> top_of_book_imbalance;
    std::optional<double> depth_imbalance;
};

struct TradeMetrics {
    Quantity traded_volume {};
    std::size_t execution_count {};
    std::optional<double> vwap;
    std::optional<double> average_trade_size;
};

struct ExecutionMetrics {
    double fill_ratio {};
    std::optional<double> slippage;
    Quantity queue_depletion {};
};

struct MarketMetrics {
    TopOfBookMetrics top_of_book;
    DepthMetrics depth;
    ImbalanceMetrics imbalance;
};

class Analytics {
public:
    static TopOfBookMetrics compute_top_of_book(const OrderBook& book);
    static DepthMetrics compute_depth(const OrderBook& book, std::size_t depth);
    static ImbalanceMetrics compute_imbalance(const OrderBook& book, std::size_t depth);
    static TradeMetrics compute_trade_metrics(const std::vector<Trade>& trades);
    static ExecutionMetrics compute_execution_metrics(
        const Event& event,
        const EngineResult& result,
        const TopOfBook& before_top,
        const TopOfBook& after_top);
    static MarketMetrics compute_market_metrics(const OrderBook& book, std::size_t depth);
};

}  // namespace ob

#include "reporting.h"

#include <filesystem>
#include <fstream>
#include <ostream>

namespace ob {

ReplayReporter::ReplayReporter(std::size_t depth) : depth_(depth) {}

RunSummary ReplayReporter::summarize(const std::vector<ReplayResult>& results, const Exchange& exchange) const {
    RunSummary summary;
    summary.total_events = results.size();
    summary.final_resting_orders = exchange.total_resting_orders();

    for (const auto& result : results) {
        switch (result.event.type) {
            case EventType::Add:
                ++summary.add_events;
                break;
            case EventType::Market:
                ++summary.market_events;
                break;
            case EventType::Cancel:
                ++summary.cancel_events;
                if (result.routed_result.cancel_succeeded) {
                    ++summary.successful_cancels;
                } else {
                    ++summary.failed_cancels;
                }
                break;
        }

        if (result.routed_result.rejected) {
            ++summary.rejected_events;
        }
        summary.execution_count += result.trade_metrics.execution_count;
        summary.traded_volume += result.trade_metrics.traded_volume;
    }

    return summary;
}

void ReplayReporter::write_trade_log(const std::filesystem::path& output_dir, const std::vector<ReplayResult>& results) const {
    std::filesystem::create_directories(output_dir);
    std::ofstream out(output_dir / "trades.csv");
    out << "event_timestamp,symbol,buy_order_id,sell_order_id,trade_price,trade_quantity,trade_timestamp\n";

    for (const auto& result : results) {
        for (const auto& trade : result.routed_result.engine_result.trades) {
            out << result.event.timestamp << ','
                << result.event.symbol << ','
                << trade.buy_order_id << ','
                << trade.sell_order_id << ','
                << trade.trade_price << ','
                << trade.trade_quantity << ','
                << trade.timestamp << '\n';
        }
    }
}

void ReplayReporter::write_rejection_log(const std::filesystem::path& output_dir, const std::vector<ReplayResult>& results) const {
    std::filesystem::create_directories(output_dir);
    std::ofstream out(output_dir / "rejections.csv");
    out << "event_timestamp,symbol,event_type,order_id,reason\n";

    for (const auto& result : results) {
        if (!result.routed_result.rejected) {
            continue;
        }
        out << result.event.timestamp << ','
            << result.event.symbol << ','
            << to_string(result.event.type) << ','
            << result.event.order_id << ','
            << result.routed_result.rejection_message << '\n';
    }
}

void ReplayReporter::write_snapshot_log(const std::filesystem::path& output_dir, const std::vector<ReplayResult>& results) const {
    std::filesystem::create_directories(output_dir);
    std::ofstream out(output_dir / "snapshots.csv");
    out << "event_timestamp,symbol,side,level_index,price,quantity,order_count\n";

    for (const auto& result : results) {
        for (std::size_t i = 0; i < result.bid_levels.size(); ++i) {
            const auto& level = result.bid_levels[i];
            out << result.event.timestamp << ',' << result.event.symbol << ",BID," << i << ','
                << level.price << ',' << level.quantity << ',' << level.order_count << '\n';
        }
        for (std::size_t i = 0; i < result.ask_levels.size(); ++i) {
            const auto& level = result.ask_levels[i];
            out << result.event.timestamp << ',' << result.event.symbol << ",ASK," << i << ','
                << level.price << ',' << level.quantity << ',' << level.order_count << '\n';
        }
    }
}

void ReplayReporter::write_state_log(
    const std::filesystem::path& output_dir,
    const std::vector<ReplayResult>& results,
    const Exchange& /* exchange */) const {
    std::filesystem::create_directories(output_dir);
    std::ofstream out(output_dir / "states.csv");
    out << "event_timestamp,symbol,order_id,status,remaining_quantity,orders_ahead,quantity_ahead,level_order_count,level_total_quantity\n";

    Exchange replay_exchange;
    for (const auto& result : results) {
        RoutedResult routed_result;
        switch (result.event.type) {
            case EventType::Add:
                routed_result = replay_exchange.submit_limit_order(
                    result.event.symbol,
                    result.event.order_id,
                    result.event.side,
                    result.event.price,
                    result.event.quantity);
                break;
            case EventType::Market:
                routed_result = replay_exchange.submit_market_order(
                    result.event.symbol,
                    result.event.order_id,
                    result.event.side,
                    result.event.quantity);
                break;
            case EventType::Cancel:
                routed_result = replay_exchange.cancel_order(result.event.order_id);
                break;
        }

        const auto* engine = replay_exchange.engine_for(result.event.symbol);
        for (const auto& state : routed_result.engine_result.state_updates) {
            out << result.event.timestamp << ','
                << result.event.symbol << ','
                << state.order_id << ','
                << to_string(state.status) << ',';

            if (state.remaining_quantity.has_value()) {
                out << *state.remaining_quantity;
            }
            out << ',';

            if (engine != nullptr) {
                if (const auto queue = engine->order_book().queue_position(state.order_id)) {
                    out << queue->orders_ahead << ','
                        << queue->quantity_ahead << ','
                        << queue->level_order_count << ','
                        << queue->level_total_quantity;
                } else {
                    out << ",,,";
                }
            } else {
                out << ",,,";
            }
            out << '\n';
        }
    }
}

void ReplayReporter::write_metrics_log(const std::filesystem::path& output_dir, const std::vector<ReplayResult>& results) const {
    std::filesystem::create_directories(output_dir);
    std::ofstream out(output_dir / "metrics.csv");
    out << "event_timestamp,symbol,best_bid,best_ask,spread,mid,bid_top_depth,ask_top_depth,total_bid,total_ask,top_imbalance,depth_imbalance,traded_volume,execution_count,fill_ratio,slippage,queue_depletion\n";

    for (const auto& result : results) {
        out << result.event.timestamp << ','
            << result.event.symbol << ',';

        if (result.market_metrics.top_of_book.best_bid) {
            out << *result.market_metrics.top_of_book.best_bid;
        }
        out << ',';
        if (result.market_metrics.top_of_book.best_ask) {
            out << *result.market_metrics.top_of_book.best_ask;
        }
        out << ',';
        if (result.market_metrics.top_of_book.spread) {
            out << *result.market_metrics.top_of_book.spread;
        }
        out << ',';
        if (result.market_metrics.top_of_book.mid_price) {
            out << *result.market_metrics.top_of_book.mid_price;
        }
        out << ','
            << result.market_metrics.depth.bid_top_n_volume << ','
            << result.market_metrics.depth.ask_top_n_volume << ','
            << result.market_metrics.imbalance.total_bid_volume << ','
            << result.market_metrics.imbalance.total_ask_volume << ',';

        if (result.market_metrics.imbalance.top_of_book_imbalance) {
            out << *result.market_metrics.imbalance.top_of_book_imbalance;
        }
        out << ',';
        if (result.market_metrics.imbalance.depth_imbalance) {
            out << *result.market_metrics.imbalance.depth_imbalance;
        }
        out << ','
            << result.trade_metrics.traded_volume << ','
            << result.trade_metrics.execution_count << ','
            << result.execution_metrics.fill_ratio << ',';
        if (result.execution_metrics.slippage) {
            out << *result.execution_metrics.slippage;
        }
        out << ','
            << result.execution_metrics.queue_depletion
            << '\n';
    }
}

void ReplayReporter::print_top_levels(const Exchange& exchange, std::ostream& out) const {
    out << "Final Top " << depth_ << " Levels\n";
    for (const auto& symbol : exchange.symbols()) {
        const auto* engine = exchange.engine_for(symbol);
        if (engine == nullptr) {
            continue;
        }
        out << "  " << symbol << '\n';
        const auto bids = engine->order_book().levels(Side::Buy, depth_);
        const auto asks = engine->order_book().levels(Side::Sell, depth_);
        out << "    Bids\n";
        for (const auto& level : bids) {
            out << "      price=" << level.price << " qty=" << level.quantity << " orders=" << level.order_count << '\n';
        }
        out << "    Asks\n";
        for (const auto& level : asks) {
            out << "      price=" << level.price << " qty=" << level.quantity << " orders=" << level.order_count << '\n';
        }
    }
}

void ReplayReporter::print_summary(const RunSummary& summary, std::ostream& out) const {
    out << "End-of-Run Summary\n";
    out << "  total_events=" << summary.total_events << '\n';
    out << "  add_events=" << summary.add_events
        << " market_events=" << summary.market_events
        << " cancel_events=" << summary.cancel_events << '\n';
    out << "  rejected_events=" << summary.rejected_events << '\n';
    out << "  successful_cancels=" << summary.successful_cancels
        << " failed_cancels=" << summary.failed_cancels << '\n';
    out << "  executions=" << summary.execution_count
        << " traded_volume=" << summary.traded_volume << '\n';
    out << "  final_resting_orders=" << summary.final_resting_orders << '\n';
}

}  // namespace ob

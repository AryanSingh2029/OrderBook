#include "reporting.h"
#include "simulator.h"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>

namespace {

void print_levels(const std::vector<ob::LevelSnapshot>& levels, const std::string& label) {
    std::cout << label << '\n';
    for (const auto& level : levels) {
        std::cout << "  price=" << level.price
                  << " qty=" << level.quantity
                  << " orders=" << level.order_count << '\n';
    }
}

void print_top(const ob::TopOfBook& top) {
    std::cout << "Top of Book\n";
    if (top.best_bid) {
        std::cout << "  bid: " << top.best_bid->price << " x " << top.best_bid->quantity << '\n';
    } else {
        std::cout << "  bid: empty\n";
    }
    if (top.best_ask) {
        std::cout << "  ask: " << top.best_ask->price << " x " << top.best_ask->quantity << '\n';
    } else {
        std::cout << "  ask: empty\n";
    }

    if (top.best_bid && top.best_ask) {
        std::cout << "  spread: " << (top.best_ask->price - top.best_bid->price) << '\n';
        const double mid = (static_cast<double>(top.best_bid->price) + static_cast<double>(top.best_ask->price)) / 2.0;
        std::cout << "  mid: " << std::fixed << std::setprecision(2) << mid << '\n';
    }
}

void print_trades(const std::vector<ob::Trade>& trades, const std::string& label) {
    std::cout << label << '\n';
    if (trades.empty()) {
        std::cout << "  no trades\n";
        return;
    }

    for (const auto& trade : trades) {
        std::cout << "  buy=" << trade.buy_order_id
                  << " sell=" << trade.sell_order_id
                  << " px=" << trade.trade_price
                  << " qty=" << trade.trade_quantity
                  << " ts=" << trade.timestamp << '\n';
    }
}

void print_book_state(
    const ob::TopOfBook& top,
    const std::vector<ob::LevelSnapshot>& bid_levels,
    const std::vector<ob::LevelSnapshot>& ask_levels) {
    print_top(top);
    print_levels(bid_levels, "Bids");
    print_levels(ask_levels, "Asks");
}

void print_market_metrics(const ob::MarketMetrics& metrics) {
    std::cout << "Market Metrics\n";
    std::cout << "  best_bid=";
    if (metrics.top_of_book.best_bid) {
        std::cout << *metrics.top_of_book.best_bid;
    } else {
        std::cout << "NA";
    }
    std::cout << " best_ask=";
    if (metrics.top_of_book.best_ask) {
        std::cout << *metrics.top_of_book.best_ask;
    } else {
        std::cout << "NA";
    }
    std::cout << '\n';

    std::cout << "  bid_top5_vol=" << metrics.depth.bid_top_n_volume
              << " ask_top5_vol=" << metrics.depth.ask_top_n_volume << '\n';
    std::cout << "  total_bid_vol=" << metrics.imbalance.total_bid_volume
              << " total_ask_vol=" << metrics.imbalance.total_ask_volume << '\n';
    std::cout << "  top_imbalance=";
    if (metrics.imbalance.top_of_book_imbalance) {
        std::cout << std::fixed << std::setprecision(4) << *metrics.imbalance.top_of_book_imbalance;
    } else {
        std::cout << "NA";
    }
    std::cout << " depth_imbalance=";
    if (metrics.imbalance.depth_imbalance) {
        std::cout << std::fixed << std::setprecision(4) << *metrics.imbalance.depth_imbalance;
    } else {
        std::cout << "NA";
    }
    std::cout << '\n';
}

void print_trade_metrics(const ob::TradeMetrics& metrics) {
    std::cout << "Trade Metrics\n";
    std::cout << "  traded_volume=" << metrics.traded_volume
              << " executions=" << metrics.execution_count;
    if (metrics.vwap) {
        std::cout << " vwap=" << std::fixed << std::setprecision(4) << *metrics.vwap;
    }
    if (metrics.average_trade_size) {
        std::cout << " avg_trade_size=" << std::fixed << std::setprecision(4) << *metrics.average_trade_size;
    }
    std::cout << '\n';
}

void print_execution_metrics(const ob::ExecutionMetrics& metrics) {
    std::cout << "Execution Metrics\n";
    std::cout << "  fill_ratio=" << std::fixed << std::setprecision(4) << metrics.fill_ratio;
    std::cout << " queue_depletion=" << metrics.queue_depletion;
    std::cout << " slippage=";
    if (metrics.slippage) {
        std::cout << std::fixed << std::setprecision(4) << *metrics.slippage;
    } else {
        std::cout << "NA";
    }
    std::cout << '\n';
}

void print_state_updates(const std::vector<ob::OrderState>& updates, const ob::OrderBook& book) {
    if (updates.empty()) {
        return;
    }

    std::cout << "States\n";
    for (const auto& state : updates) {
        std::cout << "  order=" << state.order_id
                  << " status=" << ob::to_string(state.status);
        if (state.remaining_quantity.has_value()) {
            std::cout << " remaining=" << *state.remaining_quantity;
        }
        if (state.status == ob::OrderStatus::Active) {
            if (const auto queue = book.queue_position(state.order_id)) {
                std::cout << " ahead_orders=" << queue->orders_ahead
                          << " ahead_qty=" << queue->quantity_ahead;
            }
        }
        std::cout << '\n';
    }
}

void run_events(std::istream& input, bool debug_mode, const std::filesystem::path& output_dir) {
    ob::Simulator simulator;
    ob::Exchange exchange;
    ob::ReplayReporter reporter(5);
    const auto events = simulator.load_events(input);
    const auto results = simulator.replay(events, exchange);

    if (debug_mode) {
        for (const auto& result : results) {
            std::cout << "Event: ts=" << result.event.timestamp
                      << " symbol=" << result.event.symbol
                      << " type=" << ob::to_string(result.event.type)
                      << " order_id=" << result.event.order_id << '\n';
            if (result.routed_result.rejected) {
                std::cout << "Rejected: " << result.routed_result.rejection_message << '\n';
            } else if (result.event.type == ob::EventType::Cancel) {
                std::cout << "Cancel: " << (result.routed_result.cancel_succeeded ? "accepted" : "not_found") << '\n';
            } else {
                print_trades(result.routed_result.engine_result.trades, "Trades");
                if (const auto* engine = exchange.engine_for(result.event.symbol)) {
                    print_state_updates(result.routed_result.engine_result.state_updates, engine->order_book());
                } else {
                    print_state_updates(result.routed_result.engine_result.state_updates, ob::OrderBook {});
                }
                print_trade_metrics(result.trade_metrics);
                print_execution_metrics(result.execution_metrics);
            }
            print_book_state(result.top_of_book, result.bid_levels, result.ask_levels);
            print_market_metrics(result.market_metrics);
            std::cout << '\n';
        }
    }

    reporter.write_trade_log(output_dir, results);
    reporter.write_rejection_log(output_dir, results);
    reporter.write_snapshot_log(output_dir, results);
    reporter.write_state_log(output_dir, results, exchange);
    reporter.write_metrics_log(output_dir, results);

    const auto summary = reporter.summarize(results, exchange);
    reporter.print_summary(summary, std::cout);
    reporter.print_top_levels(exchange, std::cout);
    std::cout << "  logs_dir=" << output_dir.string() << '\n';
}

}  // namespace

int main(int argc, char** argv) {
    std::string input_path = "data/sample_events.csv";
    bool debug_mode = true;
    std::filesystem::path output_dir = "logs";

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--mode=summary") {
            debug_mode = false;
        } else if (arg == "--mode=debug") {
            debug_mode = true;
        } else if (arg.rfind("--output-dir=", 0) == 0) {
            output_dir = arg.substr(std::string("--output-dir=").size());
        } else {
            input_path = arg;
        }
    }

    std::ifstream input(input_path);
    if (!input) {
        std::cerr << "Failed to open event file: " << input_path << '\n';
        return 1;
    }
    run_events(input, debug_mode, output_dir);
    return 0;
}

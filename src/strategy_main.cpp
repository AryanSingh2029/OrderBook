#include "simulator.h"
#include "strategy.h"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <filesystem>
#include <string>

namespace {

void print_strategy_summary(const ob::StrategyRunResult& result) {
    std::cout << "Strategy Sandbox Summary\n";
    std::cout << "  external_events=" << result.stats.external_events_processed << '\n';
    std::cout << "  signals_triggered=" << result.stats.signals_triggered
              << " orders_sent=" << result.stats.orders_sent
              << " filled_orders=" << result.stats.filled_orders << '\n';
    std::cout << "  requested_qty=" << result.stats.requested_quantity
              << " filled_qty=" << result.stats.filled_quantity
              << " fill_ratio=" << std::fixed << std::setprecision(4) << result.stats.fill_ratio << '\n';
    std::cout << "  inventory=" << result.stats.inventory
              << " avg_cost=" << std::fixed << std::setprecision(4) << result.stats.average_cost << '\n';
    std::cout << "  realized_pnl=" << std::fixed << std::setprecision(4) << result.stats.realized_pnl;
    if (result.stats.unrealized_pnl.has_value()) {
        std::cout << " unrealized_pnl=" << *result.stats.unrealized_pnl;
    } else {
        std::cout << " unrealized_pnl=NA";
    }
    if (result.stats.total_pnl.has_value()) {
        std::cout << " total_pnl=" << *result.stats.total_pnl;
    } else {
        std::cout << " total_pnl=NA";
    }
    std::cout << '\n';
    std::cout << "  adverse_selection_events=" << result.stats.adverse_selection_events
              << " adverse_selection_notional=" << std::fixed << std::setprecision(4)
              << result.stats.adverse_selection_notional << '\n';
}

}  // namespace

int main(int argc, char** argv) {
    std::string input_path = "data/sample_events.csv";
    std::filesystem::path output_dir = "logs";
    if (argc > 1) {
        input_path = argv[1];
    }

    std::ifstream input(input_path);
    if (!input) {
        std::cerr << "Failed to open event file: " << input_path << '\n';
        return 1;
    }

    ob::Simulator simulator;
    const auto events = simulator.load_events(input);

    ob::ImbalanceTakerStrategy strategy;
    const auto result = strategy.run(events);
    print_strategy_summary(result);

    std::filesystem::create_directories(output_dir);
    std::ofstream out(output_dir / "strategy_summary.json");
    out << "{\n"
        << "  \"external_events\": " << result.stats.external_events_processed << ",\n"
        << "  \"signals_triggered\": " << result.stats.signals_triggered << ",\n"
        << "  \"orders_sent\": " << result.stats.orders_sent << ",\n"
        << "  \"filled_orders\": " << result.stats.filled_orders << ",\n"
        << "  \"requested_qty\": " << result.stats.requested_quantity << ",\n"
        << "  \"filled_qty\": " << result.stats.filled_quantity << ",\n"
        << "  \"fill_ratio\": " << result.stats.fill_ratio << ",\n"
        << "  \"inventory\": " << result.stats.inventory << ",\n"
        << "  \"avg_cost\": " << result.stats.average_cost << ",\n"
        << "  \"realized_pnl\": " << result.stats.realized_pnl << ",\n";
    if (result.stats.unrealized_pnl) {
        out << "  \"unrealized_pnl\": " << *result.stats.unrealized_pnl << ",\n";
    } else {
        out << "  \"unrealized_pnl\": null,\n";
    }
    if (result.stats.total_pnl) {
        out << "  \"total_pnl\": " << *result.stats.total_pnl << ",\n";
    } else {
        out << "  \"total_pnl\": null,\n";
    }
    out << "  \"adverse_selection_events\": " << result.stats.adverse_selection_events << ",\n"
        << "  \"adverse_selection_notional\": " << result.stats.adverse_selection_notional << "\n"
        << "}\n";

    return 0;
}

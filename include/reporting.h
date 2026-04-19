#pragma once

#include "exchange.h"
#include "simulator.h"

#include <filesystem>
#include <iosfwd>
#include <string>

namespace ob {

struct RunSummary {
    std::size_t total_events {};
    std::size_t add_events {};
    std::size_t market_events {};
    std::size_t cancel_events {};
    std::size_t rejected_events {};
    std::size_t successful_cancels {};
    std::size_t failed_cancels {};
    std::size_t execution_count {};
    Quantity traded_volume {};
    std::size_t final_resting_orders {};
};

class ReplayReporter {
public:
    explicit ReplayReporter(std::size_t depth = 5);

    [[nodiscard]] RunSummary summarize(const std::vector<ReplayResult>& results, const Exchange& exchange) const;
    void write_trade_log(const std::filesystem::path& output_dir, const std::vector<ReplayResult>& results) const;
    void write_rejection_log(const std::filesystem::path& output_dir, const std::vector<ReplayResult>& results) const;
    void write_snapshot_log(const std::filesystem::path& output_dir, const std::vector<ReplayResult>& results) const;
    void write_state_log(
        const std::filesystem::path& output_dir,
        const std::vector<ReplayResult>& results,
        const Exchange& exchange) const;
    void write_metrics_log(const std::filesystem::path& output_dir, const std::vector<ReplayResult>& results) const;
    void print_top_levels(const Exchange& exchange, std::ostream& out) const;
    void print_summary(const RunSummary& summary, std::ostream& out) const;

private:
    std::size_t depth_ {};
};

}  // namespace ob

#include "matching_engine.h"
#include "simulator.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <filesystem>
#include <string>
#include <sys/resource.h>
#include <vector>

namespace {

struct Scenario {
    std::string name;
    std::size_t event_count {};
};

struct BenchmarkResult {
    std::string name;
    std::size_t events_processed {};
    double total_ms {};
    double throughput_ops_per_sec {};
    std::uint64_t average_latency_ns {};
    std::uint64_t worst_case_latency_ns {};
    std::uint64_t p50_latency_ns {};
    std::uint64_t p99_latency_ns {};
    double peak_memory_mb {};
    std::size_t resting_orders {};
    std::vector<std::pair<std::string, std::size_t>> latency_histogram;
};

std::uint64_t percentile(std::vector<std::uint64_t>& values, double p) {
    if (values.empty()) {
        return 0;
    }
    const std::size_t index = static_cast<std::size_t>(p * static_cast<double>(values.size() - 1));
    std::nth_element(values.begin(), values.begin() + index, values.end());
    return values[index];
}

double peak_memory_mb() {
    rusage usage {};
    if (getrusage(RUSAGE_SELF, &usage) != 0) {
        return 0.0;
    }
#if defined(__APPLE__)
    return static_cast<double>(usage.ru_maxrss) / (1024.0 * 1024.0);
#else
    return static_cast<double>(usage.ru_maxrss) / 1024.0;
#endif
}

std::vector<std::pair<std::string, std::size_t>> build_latency_histogram(const std::vector<std::uint64_t>& latencies) {
    struct Bucket {
        std::string label;
        std::uint64_t upper_bound_ns {};
    };

    const std::vector<Bucket> buckets {
        {"<=100ns", 100},
        {"101-250ns", 250},
        {"251-500ns", 500},
        {"501ns-1us", 1'000},
        {"1-2us", 2'000},
        {"2-5us", 5'000},
        {"5-10us", 10'000},
        {">10us", std::numeric_limits<std::uint64_t>::max()},
    };

    std::vector<std::pair<std::string, std::size_t>> histogram;
    histogram.reserve(buckets.size());
    for (const auto& bucket : buckets) {
        histogram.push_back({bucket.label, 0});
    }

    for (const auto latency : latencies) {
        for (std::size_t i = 0; i < buckets.size(); ++i) {
            if (latency <= buckets[i].upper_bound_ns) {
                ++histogram[i].second;
                break;
            }
        }
    }

    return histogram;
}

std::vector<ob::Event> generate_events(std::size_t event_count) {
    constexpr std::int64_t base_price = 10000;

    std::vector<ob::Event> events;
    events.reserve(event_count);

    std::vector<ob::OrderId> live_orders;
    live_orders.reserve(event_count);

    std::mt19937_64 rng(42);
    std::uniform_int_distribution<int> side_dist(0, 1);
    std::uniform_int_distribution<int> offset_dist(-10, 10);
    std::uniform_int_distribution<int> qty_dist(1, 100);
    std::uniform_int_distribution<int> action_dist(0, 99);

    ob::OrderId next_order_id = 1;
    ob::Timestamp next_timestamp = 1;

    for (std::size_t i = 0; i < event_count; ++i) {
        const int action = action_dist(rng);
        if ((action < 75) || live_orders.empty()) {
            const ob::OrderId id = next_order_id++;
            const ob::Side side = side_dist(rng) == 0 ? ob::Side::Buy : ob::Side::Sell;
            const ob::Price price = base_price + offset_dist(rng);
            const ob::Quantity qty = static_cast<ob::Quantity>(qty_dist(rng));

            events.push_back(ob::Event {
                .timestamp = next_timestamp++,
                .type = ob::EventType::Add,
                .order_id = id,
                .side = side,
                .price = price,
                .quantity = qty,
            });
            live_orders.push_back(id);
            continue;
        }

        if (action < 90) {
            const ob::OrderId id = next_order_id++;
            const ob::Side side = side_dist(rng) == 0 ? ob::Side::Buy : ob::Side::Sell;
            const ob::Quantity qty = static_cast<ob::Quantity>(qty_dist(rng));

            events.push_back(ob::Event {
                .timestamp = next_timestamp++,
                .type = ob::EventType::Market,
                .order_id = id,
                .side = side,
                .price = 0,
                .quantity = qty,
            });
            continue;
        }

        std::uniform_int_distribution<std::size_t> live_idx_dist(0, live_orders.size() - 1);
        const std::size_t idx = live_idx_dist(rng);
        const ob::OrderId id = live_orders[idx];

        events.push_back(ob::Event {
            .timestamp = next_timestamp++,
            .type = ob::EventType::Cancel,
            .order_id = id,
            .side = ob::Side::Buy,
            .price = 0,
            .quantity = 0,
        });

        live_orders[idx] = live_orders.back();
        live_orders.pop_back();
    }

    return events;
}

BenchmarkResult run_scenario(const Scenario& scenario) {
    const auto events = generate_events(scenario.event_count);

    ob::MatchingEngine engine;
    std::vector<std::uint64_t> latencies;
    latencies.reserve(events.size());

    const auto wall_start = std::chrono::steady_clock::now();

    for (const auto& event : events) {
        const auto start = std::chrono::steady_clock::now();
        switch (event.type) {
            case ob::EventType::Add:
                (void)engine.submit_limit_order(event.order_id, event.side, event.price, event.quantity);
                break;
            case ob::EventType::Market:
                (void)engine.submit_market_order(event.order_id, event.side, event.quantity);
                break;
            case ob::EventType::Cancel:
                (void)engine.cancel_order(event.order_id);
                break;
        }
        const auto end = std::chrono::steady_clock::now();
        latencies.push_back(
            static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count()));
    }

    const auto wall_end = std::chrono::steady_clock::now();
    const auto total_ns =
        static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(wall_end - wall_start).count());
    const double total_ms = static_cast<double>(total_ns) / 1'000'000.0;
    const double throughput =
        total_ns > 0 ? (static_cast<double>(events.size()) * 1'000'000'000.0) / static_cast<double>(total_ns) : 0.0;

    const auto sum_ns = std::accumulate(latencies.begin(), latencies.end(), std::uint64_t {0});
    auto latency_copy = latencies;
    const auto p50 = percentile(latency_copy, 0.50);
    latency_copy = latencies;
    const auto p99 = percentile(latency_copy, 0.99);

    return BenchmarkResult {
        .name = scenario.name,
        .events_processed = events.size(),
        .total_ms = total_ms,
        .throughput_ops_per_sec = throughput,
        .average_latency_ns = latencies.empty() ? 0 : (sum_ns / latencies.size()),
        .worst_case_latency_ns = latencies.empty() ? 0 : *std::max_element(latencies.begin(), latencies.end()),
        .p50_latency_ns = p50,
        .p99_latency_ns = p99,
        .peak_memory_mb = peak_memory_mb(),
        .resting_orders = engine.order_book().order_count(),
        .latency_histogram = build_latency_histogram(latencies),
    };
}

void print_result(const BenchmarkResult& result) {
    std::cout << result.name << '\n';
    std::cout << "  events processed: " << result.events_processed << '\n';
    std::cout << "  total runtime (ms): " << std::fixed << std::setprecision(3) << result.total_ms << '\n';
    std::cout << "  throughput (events/s): " << static_cast<std::uint64_t>(result.throughput_ops_per_sec) << '\n';
    std::cout << "  average latency (ns): " << result.average_latency_ns << '\n';
    std::cout << "  worst-case latency (ns): " << result.worst_case_latency_ns << '\n';
    std::cout << "  p50 latency (ns): " << result.p50_latency_ns << '\n';
    std::cout << "  p99 latency (ns): " << result.p99_latency_ns << '\n';
    std::cout << "  peak memory (MB): " << std::fixed << std::setprecision(3) << result.peak_memory_mb << '\n';
    std::cout << "  resting orders at end: " << result.resting_orders << '\n';
    std::cout << "  latency histogram:\n";
    for (const auto& [label, count] : result.latency_histogram) {
        const double pct = result.events_processed > 0
            ? (100.0 * static_cast<double>(count)) / static_cast<double>(result.events_processed)
            : 0.0;
        std::cout << "    " << std::setw(10) << std::left << label
                  << " : " << std::setw(8) << std::right << count
                  << " (" << std::fixed << std::setprecision(2) << pct << "%)\n";
    }
}

void write_benchmark_json(
    const std::filesystem::path& output_path,
    const std::vector<BenchmarkResult>& results) {
    std::filesystem::create_directories(output_path.parent_path());
    std::ofstream out(output_path);
    out << "{\n  \"scenarios\": [\n";
    for (std::size_t i = 0; i < results.size(); ++i) {
        const auto& result = results[i];
        out << "    {\n"
            << "      \"name\": \"" << result.name << "\",\n"
            << "      \"events_processed\": " << result.events_processed << ",\n"
            << "      \"total_ms\": " << result.total_ms << ",\n"
            << "      \"throughput\": " << result.throughput_ops_per_sec << ",\n"
            << "      \"average_latency_ns\": " << result.average_latency_ns << ",\n"
            << "      \"worst_case_latency_ns\": " << result.worst_case_latency_ns << ",\n"
            << "      \"p50_latency_ns\": " << result.p50_latency_ns << ",\n"
            << "      \"p99_latency_ns\": " << result.p99_latency_ns << ",\n"
            << "      \"peak_memory_mb\": " << result.peak_memory_mb << ",\n"
            << "      \"resting_orders\": " << result.resting_orders << ",\n"
            << "      \"latency_histogram\": [\n";
        for (std::size_t j = 0; j < result.latency_histogram.size(); ++j) {
            const auto& [label, count] = result.latency_histogram[j];
            out << "        {\"label\": \"" << label << "\", \"count\": " << count << "}";
            if (j + 1 != result.latency_histogram.size()) {
                out << ',';
            }
            out << '\n';
        }
        out << "      ]\n"
            << "    }";
        if (i + 1 != results.size()) {
            out << ',';
        }
        out << '\n';
    }
    out << "  ]\n}\n";
}

}  // namespace

int main() {
    const std::vector<Scenario> scenarios {
        {.name = "Small correctness run", .event_count = 500},
        {.name = "Medium load", .event_count = 50000},
        {.name = "Heavy load", .event_count = 500000},
    };

    std::cout << "Order Book Benchmark Report\n\n";
    std::vector<BenchmarkResult> results;
    for (const auto& scenario : scenarios) {
        const auto result = run_scenario(scenario);
        results.push_back(result);
        print_result(result);
        std::cout << '\n';
    }

    write_benchmark_json("logs/benchmark_summary.json", results);

    return 0;
}
